```mermaid
sequenceDiagram
    participant Boot as Core0Task0 (boot)
    participant ConfigManager
    participant FatFS as FileHandler/FatFS
    participant Settings as SettingsHandler

    Boot->>ConfigManager: ConfigManager_Init(filename, &mountStatus)
    Boot->>FileHandler: FatFS_SD_Mount()
    FileHandler-->>Boot: mountStatus = RES_OK
    Boot->>ConfigManager: ConfigManager_Initialize()
    Boot->>ConfigManager: ConfigManager_LoadConfig(&AppConfig)
    ConfigManager->>FatFS: Open CONF.TXT (read)
    FatFS-->>ConfigManager: raw bytes
    ConfigManager->>Settings: DeserializeHook(buffer, len, &AppConfig)
    Settings-->>ConfigManager: SETTINGS_OK
    ConfigManager-->>Boot: CONFIG_OK (AppConfig populated)
    Boot->>Settings: SettingsHandler_Init(&AppConfig)
```
sequenceDiagram
    participant User as Web UI
    participant HTTP as LwIP httpd CGI
    participant Settings as SettingsHandler
    participant ConfigMgr as ConfigManager
    participant Storage as FatFS (CONF.TXT)

    User->>HTTP: POST /config (baud1=1000000,…,action=apply)
    HTTP->>Settings: consume_param_values(key,value)
    Settings->>Settings: Update AppConfig fields\nset updated flag
    alt action == "apply"
        Settings->>HTTP: SettingsHandler_ApplyRequestCallback()
        HTTP->>Core0Task0: Notified via callback
    end
    Core0Task0->>Settings: SettingsHandler_Poll(&AppConfig)
    opt updated == 1
    Core0Task0->>ConfigMgr: ConfigManager_SaveConfig(&AppConfig)
        ConfigMgr->>Settings: SerializeHook(AppConfig, buffer,…)
        Settings-->>ConfigMgr: JSON blob + length
        ConfigMgr->>Storage: overwrite CONF.TXT
        Storage-->>ConfigMgr: FR_OK
        ConfigMgr-->>Core0Task0: CONFIG_OK
        Core0Task0->>Settings: clear updated flag
    end
```

Key effects: HTTP never touches the filesystem, `SettingsHandler_Poll` acts as the dirty-bit gate, and `ConfigManager_SaveConfig` performs an overwrite + close so SD-card updates remain atomic.
