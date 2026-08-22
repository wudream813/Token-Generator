# Token-Generator 🪙

一个现代、高级、高性能的 Windows 安全凭证与令牌伪造工具。

**Token-Generator** 允许安全研究人员、系统管理员和开发人员配置、伪造并利用自定义安全令牌运行进程（例如以 `NT AUTHORITY\SYSTEM`、`TrustedInstaller`、`LOCAL SERVICE` 等高级身份启动进程）。它通过直接与 LSASS 进程交互，并调用未公开的 Windows NT 内核调用 `NtCreateToken`，实现真正的安全上下文锻造。

---

## 🧠 Windows 内存凭证与自定义安全令牌锻造深度解析 (Windows Access Token Forgery & Deep Internals)

在 Windows NT 安全子系统中，**访问令牌 (Access Token)** 是定义进程/线程安全上下文（Security Context）的最高核心结构。它不仅包含了当前主体（Subject）的**用户安全标识符 (User SID)**、**安全组列表 (Group SIDs)**、**强制完整性级别 (Integrity Level)**，还拥有数十项控制系统敏感操作的**特权集合 (Privileges)**。

传统的提权或越权工具多依赖于简单的令牌复制（如 `DuplicateTokenEx`），其局限性在于无法脱离已有物理令牌的限制，无法自由订制主体身份、任意注入安全组以及全量激活 35 项内核级核心特权。

**Token-Generator** 采用了一种更为底层的原生锻造方法：通过直接读取 LSASS 内存凭证进行高级线程模拟，并调用未公开的 Native API `NtCreateToken`，在内核态直接拼装并锻造出全新的、高度自定义的安全访问令牌。

以下是该方案的四大核心物理阶段与底层源码级机理的详尽拆解：

```
+----------------------------------------------------------------------------------------------------+
|                                    WINDOWS TOKEN FORGERY PIPELINE                                  |
+----------------------------------------------------------------------------------------------------+
|                                                                                                    |
|  [Phase 1: Credentials Theft]                                                                      |
|  Current Process --(SeDebugPrivilege)--> OpenProcess(LSASS) --> OpenProcessToken()                 |
|                                                                                                    |
|                                                     |                                              |
|                                                     v                                              |
|                                                                                                    |
|  [Phase 2: Thread Impersonation]                                                                   |
|  ImpersonateLoggedOnUser(LSASS_Token) => Elevates thread context to NT AUTHORITY\SYSTEM            |
|                                           (Unlocks SeCreateTokenPrivilege & SeTcbPrivilege)        |
|                                                                                                    |
|                                                     |                                              |
|                                                     v                                              |
|                                                                                                    |
|  [Phase 3: Custom Token Forgery]                                                                   |
|  Populate Structures:                                                                              |
|    - TOKEN_USER (e.g. S-1-5-18)                                                                    |
|    - TOKEN_GROUPS (Administrators, Authenticated Users, etc.)                                      |
|    - TOKEN_PRIVILEGES (35 Kernel-level Privileges Enabled)                                         |
|    - TOKEN_OWNER / TOKEN_PRIMARY_GROUP                                                             |
|    - TOKEN_SOURCE ("TOKENGEN")                                                                     |
|  Execute:                                                                                          |
|    ntdll!NtCreateToken() => Yields forged hToken                                                   |
|                                                                                                    |
|                                                     |                                              |
|                                                     v                                              |
|                                                                                                    |
|  [Phase 4: Security Modifications]                                                                 |
|    - SetTokenInformation(TokenSessionId)      => Binds token to target interactive session         |
|    - SetTokenInformation(TokenIntegrityLevel) => Writes S-1-16-X Mandatory Integrity Label         |
|    - SetTokenInformation(TokenUIAccess)       => Binds UI Access flag                              |
|                                                                                                    |
|                                                     |                                              |
|                                                     v                                              |
|                                                                                                    |
|  [Phase 5: Spawning Process]                                                                       |
|  CreateProcessAsUserA(hToken) --(Handles Inheritance for -M:Inline)--> Spawned Child Process        |
|                                                                                                    |
+----------------------------------------------------------------------------------------------------+
```

---

### 一、 第一阶段：LSASS 凭证窃取与系统级线程模拟
由于直接调用未公开内核函数 `NtCreateToken` 进行令牌锻造属于极高特权操作，调用线程必须处于 `NT AUTHORITY\SYSTEM` 安全上下文中，并持有 `SeCreateTokenPrivilege`（创建令牌特权，该特权在 Windows 用户态中默认仅分配给 `SYSTEM`、`LSASS` 等系统级守护进程，任何普通管理员均不直接持有）。

因此，程序必须首先通过 LSASS（本地安全权威子系统）凭证窃取，获取系统级令牌并进行线程模拟。

#### 1.1 `SeDebugPrivilege` 提权赋能
在进行跨进程内存与句柄操作前，当前进程需要先向其自身的令牌注入并启用调试特权 `SeDebugPrivilege`：
```cpp
EnablePrivilege(NULL, "SeDebugPrivilege");
```
`SeDebugPrivilege` 允许安全工具绕过常规的系统自主访问控制列表 (DACL)，使其有权打开任意进程（包括运行在 `SYSTEM` 空间下的高敏感系统服务）。

#### 1.2 打开 LSASS 与获取凭证令牌
调用源码级函数 `GetLsassToken()`，其底层实现包含三个步骤：
1. **定位 PID**：通过进程名检索 `lsass.exe` 进程 of the PID。
2. **进程句柄获取**：使用 `OpenProcess` 请求 `PROCESS_QUERY_INFORMATION` 权限打开 LSASS 进程。
3. **令牌句柄提取**：调用 `OpenProcessToken` 获取 LSASS 进程的 Primary Token，并申请 `TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_IMPERSONATE` 权限。
```cpp
HANDLE hLsassProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, lsassPid);
HANDLE hLsassToken = NULL;
OpenProcessToken(hLsassProc, TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_IMPERSONATE, &hLsassToken);
```

#### 1.3 切换至 SYSTEM 模拟上下文
通过 `ImpersonateLoggedOnUser` 将当前调用线程的“安全上下文”切换（模拟）为 LSASS 对应的系统级访问令牌：
```cpp
ImpersonateLoggedOnUser(hLsassToken);
```
一旦模拟成功，当前线程即在内核层面取得了 `NT AUTHORITY\SYSTEM` 的等效执行权限。此时，线程将能够成功激活并使用 `SeCreateTokenPrivilege` 与 `SeTcbPrivilege`，为下一阶段的 `NtCreateToken` 扫清安全边界障碍。

---

### 二、 第二阶段：底层内核级 `NtCreateToken` 令牌伪造
传统的 Windows API （如 `DuplicateTokenEx`）本质上是在既有访问令牌上做“减法”或微调。如果需要在一张“白纸”上从无到有地、纯手工锻造出任意安全主体（如伪造全新的用户 SID、组 SIDs 集合、并且无视组策略强行开启全部特权），就必须调用 Windows NT 系统底层未公开的系统调用：`ntdll!NtCreateToken`。

#### 2.1 函数声明与动态解析
由于 `NtCreateToken` 属于未公开 Native API，源码中通过动态解析 `ntdll.dll` 导出表获取其函数指针：
```cpp
typedef NTSTATUS(NTAPI* PNtCreateToken)(
    PHANDLE TokenHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    TOKEN_TYPE TokenType,
    PLUID AuthenticationId,
    PLARGE_INTEGER ExpirationTime,
    PTOKEN_USER User,
    PTOKEN_GROUPS Groups,
    PTOKEN_PRIVILEGES Privileges,
    PTOKEN_OWNER Owner,
    PTOKEN_PRIMARY_GROUP PrimaryGroup,
    PTOKEN_DEFAULT_DACL DefaultDacl,
    PTOKEN_SOURCE TokenSource
);
```

#### 2.2 核心输入结构体的构建细节
在源码 `CreateCustomToken()` 中，对该 API 所需的关键参数结构体进行了极其严密的手工填充：

1. **`TOKEN_USER` (用户主体定义)**：
   包含指向目标用户安全标识符 (`pUserSid`) 的指针。该 SID 可以是 `SYSTEM` (`S-1-5-18`)、`TrustedInstaller`、`LOCAL SERVICE` 或任何目标自定义用户账户。

2. **`TOKEN_GROUPS` (安全组集合构建)**：
   此结构体指明该访问令牌关联的所有安全组。源码通过向堆内存动态申请 `TOKEN_GROUPS` 空间，将其 `GroupCount` 动态扩充，并将以下安全组 SID 压入：
   - **用户 SID 自身**：属性设为 `SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_OWNER`。
   - **Logon SID (登录标识符)**：提供交互式登录关联性。
   - **Administrators 组 (`S-1-5-32-544`)**：强制写入管理员别名 SID，确保进程在管理员安全过滤链中。
   - **Authenticated Users (`S-1-5-11`) & Everyone (`S-1-1-0`)**：赋予基本的系统资源访问与继承权。
   - **强制完整性标签 (System Integrity, `S-1-16-16384`)**：作为内置完整性参考。
   - **自定义扩展安全组**：动态解析用户在 GUI 界面或 CLI 中配置的附加组（如 `S-1-5-32-545` Users 组等）。

3. **`TOKEN_PRIVILEGES` (35 项特权全激活锻造)**：
   这是本工具最核心的越权突破点。Windows 默认分配的特权一般因用户组而异，且许多特权初始处于 `Disabled` 状态。
   在手工锻造时，源码直接分配了 35 个特权槽位，将 LUID 标识（`LowPart` 从 `2` 到 `36`）直接与特权映射，并强行将其属性属性字赋予为：
   ```cpp
   pPrivs->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
   ```
   这意味着，该令牌一经诞生，便在硬件与系统内核级直接拥有**完全激活、默认启用的 35 项最高系统特权**（包括 `SeTakeOwnershipPrivilege` 夺取所有权、`SeRestorePrivilege` 任意文件恢复、`SeLoadDriverPrivilege` 内核驱动加载、`SeTcbPrivilege` 系统可信基等），实现名副其实的“黄金令牌”。

4. **`TOKEN_SOURCE` (令牌来源伪造)**：
   设置令牌生成源。源码中显式伪造来源名称为 `"TOKENGEN"`，以此标记该令牌是由安全沙盒锻造器直接向内核注册生成的。

5. **认证标识与生存期**：
   - `AuthenticationId` 设为 `SYSTEM_LUID` (`{ 0x3e7, 0 }`)，向内核宣称该主体通过了系统级别的底层认证。
   - `ExpirationTime` 设置为 `-1`（即 `0xFFFFFFFF_FFFFFFFF`），使该伪造令牌永不过期。

通过执行 `NtCreateToken`，内核在内存空间中初始化并实例化该访问令牌，返回一个极高访问权限的令牌句柄 `hNewToken`。

---

### 三、 第三阶段：强制安全完整性级别写入与安全边界跨越
安全访问令牌锻造完毕后，还必须通过 Windows 强制完整性控制 (Mandatory Integrity Control, MIC) 和用户界面特权隔离 (User Interface Privilege Isolation, UIPI) 策略的校验。

#### 3.1 强制安全完整性级别 (Mandatory Integrity Level)
Windows 将执行环境分为四个主要安全级别：
- **Untrusted (不信任)** / **Low (低)** (`S-1-16-4096`)：用于沙箱沙盒、高风险浏览器。
- **Medium (中)** (`S-1-16-8192`)：普通用户运行级别。
- **High (高)** (`S-1-16-12288`)：管理员提升运行级别。
- **System (系统)** (`S-1-16-16384`)：操作系统内核与核心服务级别。

源码在 `ExecuteSudoOperation()` 中，通过 `SetTokenInformation` 的 `TokenIntegrityLevel` 模式，动态改写访问令牌中的**强制完整性标签 (Mandatory Label)**：
```cpp
SID sid = {};
sid.Revision = SID_REVISION;
sid.SubAuthorityCount = 1;
sid.IdentifierAuthority = SECURITY_MANDATORY_LABEL_AUTHORITY;
sid.SubAuthority[0] = integrityLevel; // 对应目标 IL 编码

TOKEN_MANDATORY_LABEL tml = {};
tml.Label.Attributes = SE_GROUP_INTEGRITY;
tml.Label.Sid = &sid;

SetTokenInformation(hToken, TokenIntegrityLevel, &tml, sizeof(TOKEN_MANDATORY_LABEL) + sizeof(DWORD));
```
这一机制允许该工具既可以向上提升完整性至 `System`，也可以向下降权限制令牌为 `Low`，以在高度受限的安全沙盒中运行不可信进程。

#### 3.2 UI Access 辅助控制提权标志 (`TokenUIAccess`)
在 Windows 中，低完整性级别（或常规级别）的进程默认无法向高完整性级别的窗口发送 `WM_DROPFILES`、`WM_COPYDATA` 或其他窗口消息，这被称为 **UIPI (用户界面特权隔离)**。
然而，Windows 为屏幕阅读器等辅助工具留出了一个特权通道——**UI Access**。

当在命令行传入 `--UIAccess` 或在 GUI 勾选该选项时，程序会向伪造的令牌写入 `TokenUIAccess` 属性：
```cpp
BOOL UIAccess = TRUE;
SetTokenInformation(hToken, TokenUIAccess, &UIAccess, sizeof(BOOL));
```
一旦该标志被写入，进程即可打破 UIPI 物理隔离，实现跨越屏幕特权边界的交互窗口控制。

---

### 四、 第四阶段：新进程的令牌化拉起与现代控制台 I/O 绑定
有了定制的 `hToken` 后，最后一项工程挑战是如何完美地以此令牌衍生出新的用户态进程。

#### 4.1 会话 ID 动态绑定 (Session ID Binding)
LSASS 系统令牌默认工作在 **Session 0**（孤立的后台服务会话空间），如果直接用该令牌启动交互式程序（如 `cmd.exe`），程序会卡死在后台，无法在用户当前所在的活动桌面（通常为 **Session 1** 或 **Session 2**）上呈现。

为解决此问题，源码首先通过 `GetActiveSessionID()` 获取当前真实活跃用户的 Session ID（即物理显示桌面所在的会话 ID），并在启动进程前将其强制写入令牌中：
```cpp
SetTokenInformation(hToken, TokenSessionId, &targetSessionId, sizeof(DWORD));
```
这一步极为关键，它确保了高权限的子进程能够精准地“跨越会话边界”，投递到当前用户的桌面窗口中。

#### 4.2 终结 `-M:Inline` 内联运行死锁机制 (Handle Inheritance)
在命令行中以内联方式 (`-M:Inline`) 调起子进程时，如果简单使用 `CREATE_NEW_CONSOLE` 标志，子进程会脱离当前的 CMD 控制台，强行弹出新窗口，并可能因标准 I/O（stdin, stdout, stderr）句柄被多路复用重定向而引发**多线程/多进程读写锁死锁卡死**。

为了使子进程完美地在“当前已经打开的控制台窗口”内输出，源码进行了以下底层的句柄承袭与控制台重绘重定向：
1. **剥离 `CREATE_NEW_CONSOLE`**：当处于 Inline 模式时，创建标志仅保留 `CREATE_UNICODE_ENVIRONMENT`。
2. **强行绑定标准输入输出**：将当前控制台的标准输入、标准输出、标准错误句柄直接复制给 `STARTUPINFOA` 结构：
   ```cpp
   if (windowMode == -1) { // -M:Inline
       si.dwFlags |= STARTF_USESTDHANDLES;
       si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
       si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
       si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
   }
   ```
3. **开启句柄继承**：向系统底层调用 `CreateProcessAsUserA` 时，将 `bInheritHandles` 物理参数强制设为 `TRUE`：
   ```cpp
   CreateProcessAsUserA(
       hToken, NULL, cmdLine, NULL, NULL, (windowMode == -1) ? TRUE : FALSE,
       dwCreationFlags, lpEnv, lpCurrentDir, &si, &pi
   );
   ```
通过这一精密的工程化重构，子进程将安全地复用并继承当前控制台的标准输入/输出管道。用户在当前命令行中敲击的任何字符、子进程输出的任何文本，都会无感、实时、零延迟地在当前控制台交互呈现，彻底消除了进程悬挂和死锁。

---

## 📂 模块化目录树与文件职责说明 (Modular Codebase Architecture)

为保障系统的长期维护性、方便安全研究人员与开发者敏捷地**增减、替换、扩展功能**，项目已由单一的庞大源文件重构为以下严谨、高内聚、低耦合的多文件模块化 C++ 工程：

```text
/home/user/
├── Common.h             # 基础标准头文件声明中心
├── Globals.h/.cpp       # 全局参数与配置注册中心
├── Utils.h/.cpp         # 系统底层基础工具库
├── TokenEngine.h/.cpp   # 安全令牌锻造与提权引擎
├── ThemeEngine.h/.cpp   # Fluent UI 视觉重绘与暗黑主题引擎
├── SimConsole.h/.cpp    # Simulation Terminal 仿真等宽渲染驱动
├── UIEditor.h/.cpp      # 附加组与特权状态次级高级窗口编辑器
├── MainWindow.h/.cpp    # Win32 GDI 主窗口交互消息泵 (WndProc)
├── Main.cpp             # 主程序程序入口及命令行解析器 (main / WinMain)
├── resources.rc         # Windows GDI 资源及极简平面盾牌图标绑定脚本
├── TokenGenerator.manifest # 自动管理员 UAC 提权及 Common-Controls 控件绑定清单
├── build.bat            # Windows 环境 MinGW 本地一键静态编译脚本
└── build.sh             # Linux 环境 MinGW 交叉一键静态编译脚本
```

### 文件的详细职能解析：

#### 1. `Common.h`（基础标准头文件声明中心）
* 集中包含了系统所必需的所有底层 Windows API、NT 系统级接口、多线程及 C++ 标准库依赖。
* 动态导入并声明了未公开内核函数 `PNtCreateToken` 以及 Windows 暗黑模式底层切换函数 `PDwmSetWindowAttribute`。
* 定义了项目的核心全局结构体（如安全预设 `Preset`、特权属性描述 `PrivInfo` 和日志等级 `LogLevel`）。

#### 2. `Globals.h` / `Globals.cpp`（全局参数与配置注册中心）
* 集中定义并声明了所有跨文件访问的全局变量、控件句柄、主题画刷与字体资源。
* 注册了所有的 Win32 控件唯一 ID 标识（如 `ID_BTN_RUN`，`ID_LISTVIEW_GROUPS` 等）。
* 解耦存储了 Windows 35 项内核安全特权的英文名称及其中文状态释义对照表 `g_PrivInfos`。

#### 3. `Utils.h` / `Utils.cpp`（系统底层基础工具库）
* 封装了系统级的状态控制与安全检查函数（如 `IsUserAnAdmin` 检查自身管理员提权）。
* 提供了底层进程、SID 及安全标识符的处理函数（如 `GetPidByNameA` 查找系统服务，`GetLogonSid` 提取交互式会话，`EnablePrivilege` 单独开启调试特权）。
* 包含了控制台及 GUI 实时日志分发引擎 `Log`、`Log_ErrorCode`。

#### 4. `TokenEngine.h` / `TokenEngine.cpp`（安全访问令牌锻造与提权引擎）
* 存放了本项目的安全算法核心。负责 `GetLsassToken()` 安全提取和模拟。
* 负责 `CreateCustomToken()` 零基础内存拼接，设置会话绑定，并调用底层的系统级 `NtCreateToken` 完美组装全新黄金令牌。
* 负责 `ExecuteSudoOperation()` 的环境块绑定、UI Access 特权设置、强制安全完整性写入、承袭控制台标准 I/O 句柄，最终调用 `CreateProcessAsUserA` 派生子进程。

#### 5. `ThemeEngine.h` / `ThemeEngine.cpp`（Fluent UI 视觉重绘与暗黑主题引擎）
* 提供了 Windows 11/10 现代视觉样式支持。检测 `AppsUseLightTheme` 自适应并更新全局主题颜色。
* 采用了**窗口子类化 (SetWindowSubclass)** 技术捕获按键消息并动态接管其 `WM_PAINT`，为系统自带的平铺按钮赋予丝滑的扁平、悬停及按压颜色渐变反馈。
* 负责 `DrawCardFrame()` 画刷背景渲染，为整个界面勾勒出对称、透气、极具现代 Fluent 磨砂质感的立体功能卡片。

#### 6. `SimConsole.h` / `SimConsole.cpp`（Simulation Terminal 仿真等宽渲染驱动）
* 内嵌了中文/多字节自研等宽列对齐计算器 `GetDisplayWidth()`，消除传统 `strlen` 计算汉字导致的严重右侧终端排版错位。
* 负责 `UpdateTokenPreview()` 毫秒级提取当前已配置的身份和特权状态，高保真格式化渲染出经典、极具极客质感的 `whoami /all` 仿真预览框。

#### 7. `UIEditor.h` / `UIEditor.cpp`（附加组与特权状态次级高级窗口编辑器）
* 完全剥离了两个高级自定义窗口的绘制与循环逻辑。
* 提供了组编辑器 `GroupEditorWndProc`（支持任意自定义 SID 附加组的毫秒级注入与移出）。
* 提供了特权编辑器 `PrivilegeEditorWndProc`（支持 35 项最高核心特权的单独激活、设置为 Disabled，或者利用 `CreateRestrictedToken` 进行安全特权的物理切除剥夺）。

#### 8. `MainWindow.h` / `MainWindow.cpp`（Win32 GDI 主窗口交互消息泵）
* 仅包含最核心的主界面 Win32 消息循环控制器 `WndProc`。
* 负责主窗口及其所有下拉控件、文本输入卡片、勾选组件的初始化创建，分发重绘，响应自适应 `WM_SETTINGCHANGE` 消息，在不重启进程的前提下无缝刷新 UI 视觉皮肤。

#### 9. `Main.cpp`（主程序程序入口及命令行解析器）
* 集成了最外层命令行 CLI 运行流程。负责 `ParseCommandLine()` 处理静默参数及预设参数（如 `-Use:TrustedInstaller+`，`-M:Inline`，`-IL:Low`）。
* 在 `main()` 和 `WinMain()` 中检查 UAC 管理员运行权限，根据参数特征智能引导是输出终端 CLI 使用手册，还是显式派生自适应的 Fluent GUI 桌面窗口。

---

## 🌟 最新版重磅升级与改进 (Latest Enhancements)

1. **安全预设极简优化**
   - 移除了原版较为鸡肋且低权限的 `Normal` 预设。
   - 主程序现在**默认直接选择 System 预设**，启动即处于最强安全上下文环境，化繁为简。

2. **自适应跟随系统明暗色主题 (Dynamic System Light/Dark Theme)**
   - 程序能够**完美跟随 Windows 系统明暗主题自适应切换**！
   - 采用 Windows 11/10 沉浸式暗色 API (`DwmSetWindowAttribute`) 以及 Explorer 原生暗黑皮肤组件 (`DarkMode_Explorer`)：
     - **系统处于暗黑模式：** 呈现深灰底卡片、明亮白色文字与亮色边框。
     - **系统处于明亮模式：** 自动切换为白底卡片、深炭色文字、轻量浅灰边框以及暗蓝色主交互按钮，高级感十足。
   - **动态无感重绘：** 支持 `WM_SETTINGCHANGE` 消息广播。在程序运行期间，用户只要在 Windows 设置中切换明暗主题，程序将在**不重启的情况下瞬间自动重绘**所有卡片、边框、画刷与日志字体，丝滑无痕。

3. **高仿真 `whoami /all` 等宽实时效果预览**
   - 界面整体横向扩展至 `1005px`，左侧为配置区，右侧全新开辟了高仿真的**令牌效果实时预览框 (Simulation Console)**。
   - **GBK 字符物理显示宽度自适应对齐：** 自研 `GetDisplayWidth` 列宽对齐算法。由于中文汉字在终端与 GDI 下显示占两个字符，如果用简单 `strlen` 对齐会导致严重的中文错位。本算法根据 GBK 字符宽度智能运算填充留白，确保中英文、SID、属性等各列**完美垂直对齐**。
   - **全自动实时响应：** 当您在左侧切换预设、手动更改身份名称、修改完整性等级（IL）、甚至在弹出的编辑器中修改附加组或锁定特权时，右侧预览框都会在毫秒级时间内**全自动实时更新**！

4. **100% 消除中文乱码 (Strict Charset Translation)**
   - 针对部分交叉编译器在处理中文多字节字符（如特权编辑器的状态描述“恢复默认”、“完全移除”等）时产生的乱码问题进行了硬核治理。
   - 源码文件采用 **UTF-8 编码** 保存。编译时注入以下编译器控制标志，强行让 GCC 将 UTF-8 转换为 Windows 目标 GBK 字节码：
     `g++ -finput-charset=UTF-8 -fexec-charset=GBK ...`
   - 彻底解决了中文系统下特权编辑器、状态列、弹窗和标题栏的**乱码现象**，文字完美清晰！

5. **空间布局防遮挡微调 (Precision Pixel Layout)**
   - 完美修复了上一版本中“安全令牌自定义修改（Card 3）”的标题文字与卡片第一排输入框 `桌面工作区 (Desktop)`、`强制完整性级别 (IL)` 标题文字发生轻微重叠/遮挡的视觉 Bug。
   - 整体窗口 client 高度扩充至 `770px`，卡片内首排控件统一向下平移至少 `35px`，为卡片分类主标题预留出充足、开阔的视觉空间，排版对称，呼吸感极佳。

---

## 🛠️ 编译与生成 (Compilation)

资源文件 `resources.rc` 已将清单文件 `TokenGenerator.manifest` 绑定。编译出来的 `.exe` 将自动在双击时向 Windows 申请 UAC 管理员提权，并加载现代 Common-Controls v6 视觉样式。

### 交叉编译指令 (Linux 或本地命令行)
由于 GCC 编译库依赖顺序极其严格，请确保将 `-l` 依赖库指令置于源文件之后，执行以下一键静态链接：

1. **编译打包清单及资源文件：**
   ```bash
   x86_64-w64-mingw32-windres resources.rc -o resources.o
   ```

2. **静态联立编译多文件源码并翻译为中文 GBK 字节码：**
   ```bash
   x86_64-w64-mingw32-g++ -Ofast -Wall -Wextra -static -finput-charset=UTF-8 -fexec-charset=GBK Main.cpp Globals.cpp Utils.cpp TokenEngine.cpp ThemeEngine.cpp SimConsole.cpp UIEditor.cpp MainWindow.cpp resources.o -o TokenGenerator.exe -lwtsapi32 -luserenv -lntdll -ladvapi32 -lgdi32 -lcomctl32 -lcomdlg32 -luuid -lole32
   ```

---

## 🚀 命令行 (CLI) 自动化调用参数

为兼容脚本和安全武器化集成，程序支持完全无黑框后台静默运行：

```bash
# 以 TrustedInstaller 身份启动 cmd.exe
TokenGenerator.exe -Use:TrustedInstaller cmd.exe

# 伪造本地服务身份，并强制剥夺管理员核心特权
TokenGenerator.exe -Use:"LOCAL SERVICE+" cmd.exe

# 启动一个隐藏窗口、具有极低完整性级别的 PowerShell 进程
TokenGenerator.exe -IL:Low -M:Hide "powershell.exe -ExecutionPolicy Bypass -File agent.ps1"
```

### 命令选项参考：
```text
-U:<用户名|SID>   手动指定运行目标用户上下文身份 (默认: System)
-Use:<预设>       选择预设 (System/System+, Admin/Admin+, TrustedInstaller/TrustedInstaller+, LOCAL SERVICE/LOCAL SERVICE+, NETWORK SERVICE/NETWORK SERVICE+, DWM/DWM+)
-G:<组名|SID>     注入附加组安全上下文
-IL:<完整性等级>  强制置令牌完整性标签级别 (Untrusted, Low, Medium, Medium+, High, System)
-Remove:<特权>    强制从生成的令牌中完全切除某一安全特权
-Disabled:<特权>  强制置令牌中特定特权状态为 Disabled
-M:<窗口模式>     进程 showMode: Inline (内联命令行运行), Hide (后台隐藏), Max (最大化), Min (最小化)
-C:<工作目录>     进程初始 CWD
-d:<安全桌面>     目标 Winsta\Desktop
--UIAccess        启用 UI Access 辅助控制提权标志
--Debug           开启调试输出，打印 NtCreateToken 内核调用状态
-GUI              显式启动 Windows 11 暗黑/明亮主题自适应 GUI 界面
-h, /?, --help    输出 CLI 手册
```

---
