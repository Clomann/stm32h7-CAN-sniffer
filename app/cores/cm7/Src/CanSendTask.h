#pragma once

void CanSendTaskInit(void);

void CanSendTask_Notify(void);

void CanSendTask_SetSendingActive(_Bool active);

void CanSendTask(void *arg);
