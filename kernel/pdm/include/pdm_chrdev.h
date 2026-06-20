// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_CHRDEV_H
#define PDM_CHRDEV_H

#include "lpf/lpf_chrdev.h"

#define PDM_CHRDEV_NAME_LEN LPF_CHRDEV_NAME_LEN
#define PDM_CHRDEV_SOC_NAME_LEN LPF_CHRDEV_SOC_NAME_LEN

typedef lpf_chrdev_t pdm_chrdev_t;

#define pdm_chrdev_open lpf_chrdev_open
#define pdm_chrdev_release lpf_chrdev_release
#define pdm_chrdev_register lpf_chrdev_register
#define pdm_chrdev_register_instance lpf_chrdev_register_instance
#define pdm_chrdev_register_lpf_device lpf_chrdev_register_lpf_device
#define pdm_chrdev_unregister lpf_chrdev_unregister
#define pdm_chrdev_from_file lpf_chrdev_from_file
#define pdm_chrdev_open_count lpf_chrdev_open_count
#define pdm_chrdev_error_count lpf_chrdev_error_count
#define pdm_chrdev_record_error lpf_chrdev_record_error
#define pdm_chrdev_index lpf_chrdev_index

#endif /* PDM_CHRDEV_H */
