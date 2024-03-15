/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef __CORE_PLF_INIT_H__
#define __CORE_PLF_INIT_H__

extern struct miscdevice met_device;

/*
 *   MET External Symbol
 */

#ifdef MET_GPU
/*
 *   GPU
 */
#include <mtk_gpu_utility.h>
#include <mtk_gpufreq.h>
#include "met_gpu_monitor.h"

extern bool mtk_get_gpu_loading(unsigned int *pLoading);
extern bool mtk_get_gpu_block(unsigned int *pBlock);
extern bool mtk_get_gpu_idle(unsigned int *pIdle);
extern bool mtk_get_gpu_dvfs_from(MTK_GPU_DVFS_TYPE *peType, unsigned long *pulFreq);
extern bool mtk_get_gpu_sub_loading(unsigned int *pLoading);
extern bool mtk_get_3D_fences_count(int *pi32Count);
extern bool mtk_get_gpu_memory_usage(unsigned int *pMemUsage);
extern bool mtk_get_gpu_power_loading(unsigned int *pLoading);
extern bool mtk_get_custom_boost_gpu_freq(unsigned int *pui32FreqLevel);
extern bool mtk_get_custom_upbound_gpu_freq(unsigned int *pui32FreqLevel);
extern bool mtk_get_vsync_based_target_freq(unsigned long *pulFreq);
extern bool mtk_get_vsync_offset_event_status(unsigned int *pui32EventStatus);
extern bool mtk_get_vsync_offset_debug_status(unsigned int *pui32EventStatus);
extern bool mtk_enable_gpu_perf_monitor(bool enable);
extern bool mtk_get_gpu_pmu_init(GPU_PMU *pmus, int pmu_size, int *ret_size);
extern bool mtk_get_gpu_pmu_swapnreset(GPU_PMU *pmus, int pmu_size);

extern bool (*mtk_get_gpu_loading_symbol)(unsigned int *pLoading);
extern bool (*mtk_get_gpu_block_symbol)(unsigned int *pBlock);
extern bool (*mtk_get_gpu_idle_symbol)(unsigned int *pIdle);
extern bool (*mtk_get_gpu_dvfs_from_symbol)(MTK_GPU_DVFS_TYPE *peType, unsigned long *pulFreq);
extern bool (*mtk_get_gpu_sub_loading_symbol)(unsigned int *pLoading);
extern bool (*mtk_get_3D_fences_count_symbol)(int *pi32Count);
extern bool (*mtk_get_gpu_memory_usage_symbol)(unsigned int *pMemUsage);
extern bool (*mtk_get_gpu_power_loading_symbol)(unsigned int *pLoading);
extern bool (*mtk_get_custom_boost_gpu_freq_symbol)(unsigned long *pulFreq);
extern bool (*mtk_get_custom_upbound_gpu_freq_symbol)(unsigned long *pulFreq);
extern bool (*mtk_get_vsync_based_target_freq_symbol)(unsigned long *pulFreq);
extern bool (*mtk_get_vsync_offset_event_status_symbol)(unsigned int *pui32EventStatus);
extern bool (*mtk_get_vsync_offset_debug_status_symbol)(unsigned int *pui32EventStatus);
extern bool (*mtk_enable_gpu_perf_monitor_symbol)(bool enable);
extern bool (*mtk_get_gpu_pmu_init_symbol)(GPU_PMU *pmus, int pmu_size, int *ret_size);
extern bool (*mtk_get_gpu_pmu_swapnreset_symbol)(GPU_PMU *pmus, int pmu_size);
extern bool (*mtk_get_gpu_pmu_deinit_symbol)(void);
extern bool (*mtk_get_gpu_pmu_swapnreset_stop_symbol)(void);

extern bool mtk_register_gpu_power_change(const char *name, gpu_power_change_notify_fp callback);
extern bool mtk_unregister_gpu_power_change(const char *name);
extern bool (*mtk_register_gpu_power_change_symbol)(const char *name,
					gpu_power_change_notify_fp callback);
extern bool (*mtk_unregister_gpu_power_change_symbol)(const char *name);


extern unsigned int (*mt_gpufreq_get_cur_freq_symbol)(void);
extern unsigned int (*mt_gpufreq_get_thermal_limit_freq_symbol)(void);

extern struct metdevice met_gpu;
extern struct metdevice met_gpudvfs;
extern struct metdevice met_gpumem;
extern struct metdevice met_gpupwr;
extern struct metdevice met_gpu_pmu;
#ifdef MET_GPU_STALL_MONITOR
extern struct metdevice met_gpu_stall;
#endif
#endif /* MET_GPU */


#ifdef MET_VCOREDVFS
/*
 *   VCORE DVFS
 */
extern int vcorefs_get_num_opp(void);
extern int  vcorefs_get_opp_info_num(void);
extern char ** vcorefs_get_opp_info_name(void);
extern unsigned int * vcorefs_get_opp_info(void);
extern int  vcorefs_get_src_req_num(void);
extern char ** vcorefs_get_src_req_name(void);
extern unsigned int * vcorefs_get_src_req(void);

extern int (*vcorefs_get_num_opp_symbol)(void);
extern int  (*vcorefs_get_opp_info_num_symbol)(void);
extern char ** (*vcorefs_get_opp_info_name_symbol)(void);
extern unsigned int * (*vcorefs_get_opp_info_symbol)(void);
extern int  (*vcorefs_get_src_req_num_symbol)(void);
extern char ** (*vcorefs_get_src_req_name_symbol)(void);
extern unsigned int * (*vcorefs_get_src_req_symbol)(void);

extern struct metdevice met_vcoredvfs;
#endif /* MET_VCOREDVFS */


#ifdef MET_EMI
extern void *mt_cen_emi_base_get(void);
extern unsigned int get_dram_data_rate(void);      /* in Mhz */
extern int get_ddr_type(void);
extern void *get_cur_ddr_ratio(void);

extern void *(*mt_cen_emi_base_get_symbol)(void);
extern unsigned int (*get_dram_data_rate_symbol)(void); /* in Mhz */
extern unsigned int (*get_ddr_type_symbol)(void);
extern unsigned int (*get_cur_ddr_ratio_symbol)(void);



extern struct metdevice met_sspm_emi;
#endif /* MET_EMI */


#ifdef MET_PTPOD
#include <mtk_gpufreq.h>
#include <mach/mtk_cpufreq_api.h>
#include <mtk_cpufreq_config.h>

extern unsigned int (*mt_gpufreq_get_cur_volt_symbol)(void);
extern unsigned int (*mt_cpufreq_get_cur_volt_symbol)(unsigned int cluster_id);

extern struct metdevice met_ptpod;
#endif /* MET_PTPOD */

#ifdef MET_WALLTIME
extern struct metdevice met_wall_time;
#endif

#ifdef MTK_TINYSYS_SSPM_SUPPORT
extern struct metdevice met_sspm_common;
#endif /* MTK_TINYSYS_SSPM_SUPPORT */

#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) && defined(ONDIEMET_SUPPORT)
#ifdef MET_SSPM_WALLTIME
extern struct metdevice met_sspm_walltime;
#endif
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

#ifdef MET_CPUDSU
extern struct metdevice met_cpudsu;
#endif


#endif /*__CORE_PLF_INIT_H__*/
