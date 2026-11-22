import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1180
    height: 760
    visible: true
    title: "UnifiedStreamNext"

    Material.theme: connectionBridge.darkMode ? Material.Dark : Material.Light
    Material.accent: Material.Indigo

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Pane {
            Layout.fillHeight: true
            Layout.preferredWidth: 220
            Material.elevation: 2

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                Label {
                    text: "UnifiedStreamNext"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.margins: 12
                }

                Repeater {
                    model: ["首页", "设备列表", "添加设备", "连接管理", "连接历史", "设置", "关于"]
                    delegate: ItemDelegate {
                        Layout.fillWidth: true
                        text: modelData
                        highlighted: navStack.currentIndex === index
                        onClicked: navStack.currentIndex = index
                    }
                }

                Item { Layout.fillHeight: true }

                Switch {
                    text: "深色模式"
                    checked: connectionBridge.darkMode
                    onToggled: connectionBridge.darkMode = checked
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Live status banner from ConnectionManager
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: connectionBridge.lastStatusMessage.length > 0 ? 38 : 0
                visible: connectionBridge.lastStatusMessage.length > 0
                color: Material.accent

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    color: "white"
                    text: connectionBridge.lastStatusMessage
                }
            }

            StackLayout {
                id: navStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: 0

                // 0: Home
                Pane {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 16
                        Label { text: "首页"; font.pixelSize: 26; font.bold: true }
                        RowLayout {
                            spacing: 12
                            Button { text: "扫描局域网设备"; highlighted: true; onClicked: connectionBridge.discoverDevices() }
                            Button { text: "手动添加设备"; onClicked: navStack.currentIndex = 2 }
                        }
                        Label { text: "已保存设备数量：" + connectionBridge.devices.length; font.pixelSize: 16 }
                        Item { Layout.fillHeight: true }
                    }
                }

                // 1: Device List
                Pane {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 12
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: "设备列表"; font.pixelSize: 26; font.bold: true; Layout.fillWidth: true }
                            Button { text: "刷新发现"; onClicked: connectionBridge.discoverDevices() }
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 10
                            model: connectionBridge.devices
                            delegate: Frame {
                                width: ListView.view.width
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 12
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Label { text: modelData.name + " (" + modelData.type + ")"; font.bold: true; font.pixelSize: 16 }
                                        Label { text: "系统: " + modelData.osVersion + " | LAN: " + modelData.lanAddress + " | 状态: " + modelData.state + " | 引擎: " + modelData.activeEngine }
                                        Label { visible: modelData.lastError.length > 0; text: "错误: " + modelData.lastError; color: "#D32F2F" }
                                    }
                                    Button { text: "连接"; highlighted: true; onClicked: connectionBridge.connectDevice(modelData.id) }
                                    Button { text: "断开"; onClicked: connectionBridge.disconnectDevice(modelData.id) }
                                    Button { text: "删除"; onClicked: connectionBridge.removeDevice(modelData.id) }
                                }
                            }
                        }
                    }
                }

                // 2: Add Device
                Pane {
                    ColumnLayout {
                        anchors.margins: 24
                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 480
                        spacing: 12
                        Label { text: "添加设备"; font.pixelSize: 26; font.bold: true }
                        TextField { id: nameInput; Layout.fillWidth: true; placeholderText: "设备名称（例如：主力工作站）" }
                        TextField { id: lanInput; Layout.fillWidth: true; placeholderText: "局域网 IPv4 地址（例如：192.168.1.50）" }
                        TextField { id: rdInput; Layout.fillWidth: true; placeholderText: "RustDesk Peer ID（跨网/备用，可选）" }
                        ComboBox { id: typeCombo; Layout.fillWidth: true; model: ["Windows PC", "Android 设备"] }
                        Button {
                            text: "保存设备"
                            highlighted: true
                            onClicked: {
                                if (connectionBridge.addManualDevice(nameInput.text, lanInput.text, rdInput.text, typeCombo.currentIndex)) {
                                    nameInput.text = ""; lanInput.text = ""; rdInput.text = ""
                                    navStack.currentIndex = 1
                                }
                            }
                        }
                    }
                }

                // 3: Connection Management
                Pane {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 12
                        Label { text: "连接管理"; font.pixelSize: 26; font.bold: true }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 10
                            model: connectionBridge.devices
                            delegate: Frame {
                                width: ListView.view.width
                                RowLayout {
                                    anchors.fill: parent
                                    Label { text: modelData.name + " [" + modelData.state + "]"; Layout.fillWidth: true }
                                    Button { text: "自动模式"; onClicked: connectionBridge.switchEngine(modelData.id, 0) }
                                    Button { text: "强制 Sunshine/Moonlight"; onClicked: connectionBridge.switchEngine(modelData.id, 1) }
                                    Button { text: "强制 RustDesk"; onClicked: connectionBridge.switchEngine(modelData.id, 2) }
                                    Button { text: "重连"; onClicked: connectionBridge.reconnectDevice(modelData.id) }
                                }
                            }
                        }
                    }
                }

                // 4: History
                Pane {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 12
                        Label { text: "连接历史"; font.pixelSize: 26; font.bold: true }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8
                            model: connectionBridge.history
                            delegate: Frame {
                                width: ListView.view.width
                                ColumnLayout {
                                    Label { text: modelData.deviceName + " | 引擎: " + modelData.engine + " | 结果: " + modelData.finalState; font.bold: true }
                                    Label { visible: modelData.switchReason.length > 0; text: "切换原因: " + modelData.switchReason }
                                    Label { visible: modelData.errorMessage.length > 0; text: "错误详情: " + modelData.errorMessage; color: "#D32F2F" }
                                }
                            }
                        }
                    }
                }

                // 5: Settings
                Pane {
                    ColumnLayout {
                        anchors.margins: 24
                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 520
                        spacing: 12
                        Label { text: "设置 — 中继与网络策略"; font.pixelSize: 26; font.bold: true }
                        ComboBox { id: relayModeCombo; Layout.fillWidth: true; model: ["禁用中继（仅直连）", "官方公共中继（P0 预留，暂未部署）", "用户自建中继（hbbs/hbbr/TURN）"] }
                        TextField { id: rendezvousInput; Layout.fillWidth: true; placeholderText: "自建 Rendezvous (hbbs) 主机地址" }
                        TextField { id: relayHostInput; Layout.fillWidth: true; placeholderText: "自建 Relay (hbbr / TURN) 主机地址" }
                        CheckBox { id: lanDirectCheck; text: "局域网媒体流优先点对点直连（不经中继）"; checked: true }
                        Button {
                            text: "保存网络与中继设置"
                            highlighted: true
                            onClicked: connectionBridge.saveRelaySettings(
                                relayModeCombo.currentIndex, rendezvousInput.text, 21116,
                                relayHostInput.text, 21117, lanDirectCheck.checked)
                        }
                    }
                }

                // 6: About
                Pane {
                    ColumnLayout {
                        anchors.margins: 24
                        spacing: 10
                        Label { text: "关于 UnifiedStreamNext"; font.pixelSize: 26; font.bold: true }
                        Label { text: "版本：0.1.0" }
                        Label { text: "上游引擎基线：Sunshine (GPL-3.0) · Moonlight Qt/Android (GPL-3.0) · RustDesk (AGPL-3.0)" }
                        Label { text: "详细第三方声明与源码获取方式请参阅 LICENSES.md 与 THIRD_PARTY_NOTICES.txt。" }
                    }
                }
            }
        }
    }
}
