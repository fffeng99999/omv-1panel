# openmediavault-1panel

OpenMediaVault 8.x 插件：在 WebUI 里安装、管理 [1Panel](https://github.com/1Panel-dev/1Panel)（Linux 服务器管理面板）。**只使用官方二进制与官方安装脚本，不做任何编译或修改**。

## 插件工作原理

- 配置存 `conf.service.onepanel`（安装目录/Web 端口/安全入口/用户名/密码/面板语言/enable 开关）。安全入口与密码在插件安装时随机生成，页面上可直接查看。
- 「安装」按钮走 RPC 后台任务（`/usr/sbin/omv-1panel-ctl`）：
  1. 从 1Panel CDN 取最新版本号（`https://resource.fit2cloud.com/1panel/package/v2/stable/latest`）；
  2. 下载官方发行包 `1panel-<版本>-linux-<架构>.tar.gz`（架构自动映射 amd64/arm64/armv7 等）；
  3. 按 CDN `checksums.txt` 做 sha256 校验；
  4. 以非交互环境变量（`PANEL_NON_INTERACTIVE=true` 等）运行**官方** `install.sh`——明确禁用 Docker 安装与 daemon.json 改写（`PANEL_INSTALL_DOCKER=n` / `PANEL_CONFIGURE_ACCELERATOR=n` / `PANEL_REPLACE_DAEMON_JSON=n`），不碰 OMV 的 Docker。
- 「升级」执行官方 `1pctl update` 在线自升级；「卸载」用自写的非交互卸载（官方 `1pctl uninstall` 是交互式的）：停用并移除 systemd 单元与二进制，**面板数据目录保留**。
- 「应用」时 Salt state（`srv/salt/omv/deploy/onepanel/`）只管理 `1panel-core` / `1panel-agent` 两个 systemd 服务的 enabled/running（enable=false 时停止并禁用）；单元文件由官方安装器写入，插件不渲染。面板未安装时 state 自动跳过。
- 状态页实时显示：服务状态（运行/部分/停止/未安装）、已装版本、最新上游版本与更新提示；另有用户信息（面板地址/安全入口）与日志（journalctl 两个单元）查看。
- 「打开 1Panel」按 WebUI 主机名 + 端口 + 安全入口跳转。

### 红线与限制

- 插件只做 OMV 侧集成，**不改 1Panel 自身功能**；应用商店、网站、数据库、容器等业务一律在 1Panel 原生界面管理。
- 装后修改密码/用户名/安全入口请用 1Panel 原生界面或 `1pctl`（`1pctl update password` 为交互式命令，插件不做非交互 hack）。
- 若在原生界面改过端口/入口，本页显示的端口/入口为安装时快照，以「查看用户信息」输出为准。
- 「启动/停止/重启」按钮只做即时启停；开机自启由「启用」开关 + 「应用」控制。

## 测试环境实测记录（2026-10-04，`${TEST_ENV_IP_ADDRESS}`）

- `openmediavault-1panel 8.0.1`（Architecture: all，不 ship 二进制）本地构建安装；路由、中文 i18n、Salt 启停全部生效。
- RPC「安装」任务：CDN 下载 62 MB（约 5 秒）→ sha256 校验 OK → 官方安装器静默完成，10 秒内 `1panel-core` + `1panel-agent` 双服务 active，面板监听 10086，安全入口 HTTP 200。
- 启停往返（RPC start/stop + enable 开关 applyChanges）与状态/版本/更新检测（2.3.2 = 2.3.2，无更新）全部符合预期。
- 插件重装/卸载不影响已装好的 1Panel（prerm/postrm 不碰面板本身）。

## 发布流程

push tag `v*` → GitHub Actions（`.github/workflows/build.yml`）lint + 构建 `_all.deb` + 附到 Release；`${GITHUB_USER}` 占位符由 `render-vars.sh` 按 `debian/variables.env` 渲染。
