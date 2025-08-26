#include "ConfigManager.h"

struct ConfigFileManagerType
{
    char *filename;
    uint32_t fnamemaxlen;
    uint32_t count;
    uint32_t timestamp;
    uint8_t openRes;
    FatFsDeviceType writeFileDevice;
    uint8_t *mountRes;
};

static char ConfigFileName[255] = "CONF.TXT";
static ConfigFileManagerType ConfigCtrlData;

ConfigFileManagerType *ConfigManager_Init(uint8_t *mount_res)
{
    ConfigCtrlData.filename = ConfigFileName;
    ConfigCtrlData.fnamemaxlen = sizeof(ConfigFileName);
    ConfigCtrlData.count = 0;
    ConfigCtrlData.timestamp = 0;
    ConfigCtrlData.writeFileDevice.readTargetSize = 0U;
    ConfigCtrlData.mountRes = mount_res;

    return &ConfigCtrlData;
}

uint8_t ConfigManager_Load(ConfigFileManagerType *mgr)
{
    return 0;
}

uint8_t ConfigManager_Apply(ConfigFileManagerType *mgr)
{
    return 0;
}

ConfigFileManagerType *CanLogHandler_GetControlData()
{
    return &ConfigCtrlData;
}

void appConfigHandlerInit(ConfigFileManagerType *mng)
{
  if ( RES_OK == *mng->mountRes)
  {
    mng->openRes = FatFS_SD_OpenFileForWrite(
                              &(mng->writeFileDevice),
                              mng->filename);

    /* enforce f_seek to zero via custom flags to not clear content later */
    mng->writeFileDevice.fflags = FA_CREATE_ALWAYS | FA_WRITE;
  }
  else 
  {
    mng->openRes = 1U;
  }
}

FatFsDeviceType *ConfigManager_GetFile(ConfigFileManagerType *mgr)
{   
    return &mgr->writeFileDevice;
}

uint8_t ConfigManager_GetOpenRes(ConfigFileManagerType *mng)
{
    return mng->openRes;
}
