# UnifiedStreamNext 用户手册

## 1. 快速开始

1. **添加设备**：打开「添加设备」页，点击「扫描局域网设备」自动发现同网段主机，或手动输入目标设备的局域网 IPv4 地址与 RustDesk Peer ID。
2. **一键连接**：在「设备列表」点击目标设备卡片的「连接」按钮，`UnifiedStreamNext` 会自动判定网络环境：
   - **同局域网且 Sunshine 可用（模式 A）**：优先走 Sunshine + Moonlight 低延迟点对点串流。
   - **跨网络环境（模式 B）**：自动调度 RustDesk 穿透或中继建立远程桌面会话。
   - **同局域网但 Sunshine 不可用（模式 C）**：自动回退到 RustDesk 局域网连接，并在状态栏说明切换原因。
3. **手动切换与管理**：在「连接管理」页可强制切换引擎（自动 / Sunshine+Moonlight / RustDesk）或触发重连。

## 2. 自建中继节点（可选）

若需跨网稳定中继，可使用项目内置的 `deploy/relay/docker-compose.yml` 在云主机启动自建 `hbbs`/`hbbr`，随后在「设置」页选择「用户自建中继」并填写服务器地址。局域网内设备仍会优先点对点直连，不经过中继。
