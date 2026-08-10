# Token-Generator

一个现代、高级、高性能的 Windows 安全凭证与令牌伪造工具。

**Token-Generator** 允许安全研究人员、系统管理员和开发人员配置、伪造并利用自定义安全令牌运行进程（例如以 `NT AUTHORITY\SYSTEM`、`TrustedInstaller`、`LOCAL SERVICE` 等高级身份启动进程）。它通过直接与 LSASS 进程交互，并调用未公开的 Windows NT 内核调用 `NtCreateToken`，实现真正的安全上下文锻造。

---

## 最新版重磅升级与改进 (Latest Enhancements)

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

## 编译与生成 (Compilation)

资源文件 `resources.rc` 已将清单文件 `TokenGenerator.manifest` 绑定。编译出来的 `.exe` 将自动在双击时向 Windows 申请 UAC 管理员提权，并加载现代 Common-Controls v6 视觉样式。

### 交叉编译指令 (Linux 或本地命令行)
由于 GCC 编译库依赖顺序极其严格，请确保将 `-l` 依赖库指令置于源文件之后，执行以下一键静态链接：

1. **编译打包清单及资源文件：**
   ```bash
   x86_64-w64-mingw32-windres resources.rc -o resources.o
   ```

2. **静态联立编译 UTF-8 源码（自动翻译为中文 GBK 字节码）：**
   ```bash
   x86_64-w64-mingw32-g++ -Ofast -Wall -Wextra -static TokenGenerator.cpp resources.o -o TokenGenerator.exe -lwtsapi32 -luserenv -lntdll -ladvapi32 -lgdi32 -lcomctl32 -lcomdlg32 -luuid -lole32
   ```

---

## 命令行 (CLI) 自动化调用参数

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
