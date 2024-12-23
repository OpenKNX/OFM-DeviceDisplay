#pragma once
static constexpr const char* defaultMenuJson = R"({
        "menu": [
            {
                "label": "Main Settings",
                "type": "Submenu",
                "submenu": [
                    {
                        "label": "Network Settings",
                        "type": "Submenu",
                        "submenu": [
                            {
                                "label": "DHCP",
                                "type": "Checkbox",
                                "defaultValue": true
                            },
                            {
                                "label": "IP Address",
                                "type": "Action",
                                "isVisible": false
                            },
                            {
                                "label": "Subnet Mask",
                                "type": "Action",
                                "isVisible": true
                            },
                            {
                                "label": "DNS Settings",
                                "type": "Submenu",
                                "submenu": [
                                    {
                                        "label": "Primary DNS",
                                        "type": "Action"
                                    },
                                    {
                                        "label": "Secondary DNS",
                                        "type": "Action"
                                    },
                                    {
                                        "label": "Back",
                                        "type": "Back"
                                    }
                                ]
                            },
                            {
                                "label": "Back",
                                "type": "Back"
                            }
                        ]
                    },
                    {
                        "label": "System Settings",
                        "type": "Submenu",
                        "submenu": [
                            {
                                "label": "Time Zone",
                                "type": "Dropdown",
                                "defaultValue": 2,
                                "options": ["UTC-12", "UTC-11", "UTC+0", "UTC+1", "UTC+2"]
                            },
                            {
                                "label": "Date/Time",
                                "type": "Action"
                            },
                            {
                                "label": "Reboot Device",
                                "type": "Action"
                            },
                            {
                                "label": "Back",
                                "type": "Back"
                            }
                        ]
                    },
                    {
                        "label": "Device Info",
                        "type": "Submenu",
                        "submenu": [
                            {
                                "label": "Device Name",
                                "type": "Action"
                            },
                            {
                                "label": "Firmware Version",
                                "type": "Action"
                            },
                            {
                                "label": "Serial Number",
                                "type": "Action"
                            },
                            {
                                "label": "Back",
                                "type": "Back"
                            }
                        ]
                    },
                    {
                        "label": "Back",
                        "type": "Back"
                    }
                ]
            },
            {
                "label": "Advanced Settings",
                "type": "Submenu",
                "submenu": [
                    {
                        "label": "Security",
                        "type": "Submenu",
                        "submenu": [
                            {
                                "label": "Password",
                                "type": "TextInput",
                                "defaultValue": "defaultpassword"
                            },
                            {
                                "label": "Enable Encryption",
                                "type": "Checkbox",
                                "defaultValue": false
                            },
                            {
                                "label": "Back",
                                "type": "Back"
                            }
                        ]
                    },
                    {
                        "label": "Back",
                        "type": "Back"
                    }
                ]
            },
            {
                "label": "Brightness",
                "type": "Dropdown",
                "defaultValue": 2,
                "options": ["25%", "50%", "75%", "100%"]
            },
            {
                "label": "Auto Dimming",
                "type": "Checkbox",
                "defaultValue": true
            },
            {
                "label": "Screen Saver",
                "type": "Action"
            }
        ]
    })";