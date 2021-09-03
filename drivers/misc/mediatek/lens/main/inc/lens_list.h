/*
 * Copyright (C) 2015 MediaTek Inc.
 * Copyright (C) 2019 XiaoMi, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _LENS_LIST_H
#define _LENS_LIST_H

extern void MAIN2AF_PowerDown(void);

#define CACTUS_DW9714AF_OFILM_SetI2Cclient CACTUS_DW9714AF_OFILM_SetI2Cclient_Main
#define CACTUS_DW9714AF_OFILM_Ioctl CACTUS_DW9714AF_OFILM_Ioctl_Main
#define CACTUS_DW9714AF_OFILM_Release CACTUS_DW9714AF_OFILM_Release_Main
#define CACTUS_DW9714AF_OFILM_GetFileName CACTUS_DW9714AF_OFILM_GetFileName_Main
extern int CACTUS_DW9714AF_OFILM_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long CACTUS_DW9714AF_OFILM_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int CACTUS_DW9714AF_OFILM_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int CACTUS_DW9714AF_OFILM_GetFileName(unsigned char *pFileName);


#define CACTUS_FP5510E2AF_SUNNY_SetI2Cclient CACTUS_FP5510E2AF_SUNNY_SetI2Cclient_Main
#define CACTUS_FP5510E2AF_SUNNY_Ioctl CACTUS_FP5510E2AF_SUNNY_Ioctl_Main
#define CACTUS_FP5510E2AF_SUNNY_Release CACTUS_FP5510E2AF_SUNNY_Release_Main
#define CACTUS_FP5510E2AF_SUNNY_GetFileName CACTUS_FP5510E2AF_SUNNY_GetFileName_Main

extern int CACTUS_FP5510E2AF_SUNNY_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long CACTUS_FP5510E2AF_SUNNY_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int CACTUS_FP5510E2AF_SUNNY_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int CACTUS_FP5510E2AF_SUNNY_GetFileName(unsigned char *pFileName);


#define CEREUS_DW9714AF_OFILM_SetI2Cclient CEREUS_DW9714AF_OFILM_SetI2Cclient_Main
#define CEREUS_DW9714AF_OFILM_Ioctl CEREUS_DW9714AF_OFILM_Ioctl_Main
#define CEREUS_DW9714AF_OFILM_Release CEREUS_DW9714AF_OFILM_Release_Main
#define CEREUS_DW9714AF_OFILM_GetFileName CEREUS_DW9714AF_OFILM_GetFileName_Main
extern int CEREUS_DW9714AF_OFILM_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long CEREUS_DW9714AF_OFILM_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int CEREUS_DW9714AF_OFILM_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int CEREUS_DW9714AF_OFILM_GetFileName(unsigned char *pFileName);


#define CEREUS_DW9714AF_SUNNY_SetI2Cclient CEREUS_DW9714AF_SUNNY_SetI2Cclient_Main
#define CEREUS_DW9714AF_SUNNY_Ioctl CEREUS_DW9714AF_SUNNY_Ioctl_Main
#define CEREUS_DW9714AF_SUNNY_Release CEREUS_DW9714AF_SUNNY_Release_Main
#define CEREUS_DW9714AF_SUNNY_GetFileName CEREUS_DW9714AF_SUNNY_GetFileName_Main
extern int CEREUS_DW9714AF_SUNNY_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long CEREUS_DW9714AF_SUNNY_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int CEREUS_DW9714AF_SUNNY_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int CEREUS_DW9714AF_SUNNY_GetFileName(unsigned char *pFileName);


#endif
