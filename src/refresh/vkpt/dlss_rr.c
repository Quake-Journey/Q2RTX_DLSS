/*
Copyright (C) 2018 Christoph Schied
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "vkpt.h"

enum {
    DLSS_RR_PREP,
    DLSS_RR_NUM_PIPELINES
};

static VkPipeline pipelines[DLSS_RR_NUM_PIPELINES];
static VkPipelineLayout pipeline_layout_dlss_rr;

#define BARRIER_COMPUTE(cmd_buf, img) \
    do { \
        VkImageSubresourceRange subresource_range = { \
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, \
            .baseMipLevel   = 0, \
            .levelCount     = 1, \
            .baseArrayLayer = 0, \
            .layerCount     = 1 \
        }; \
        IMAGE_BARRIER(cmd_buf, \
            .image            = img, \
            .subresourceRange = subresource_range, \
            .srcAccessMask    = VK_ACCESS_SHADER_WRITE_BIT, \
            .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT, \
            .oldLayout        = VK_IMAGE_LAYOUT_GENERAL, \
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL); \
    } while (0)

VkResult vkpt_dlss_rr_initialize(void)
{
    VkDescriptorSetLayout desc_set_layouts[] = {
        qvk.desc_set_layout_ubo,
        qvk.desc_set_layout_textures,
    };

    CREATE_PIPELINE_LAYOUT(qvk.device, &pipeline_layout_dlss_rr,
        .setLayoutCount = LENGTH(desc_set_layouts),
        .pSetLayouts = desc_set_layouts);
    ATTACH_LABEL_VARIABLE(pipeline_layout_dlss_rr, PIPELINE_LAYOUT);

    return VK_SUCCESS;
}

VkResult vkpt_dlss_rr_destroy(void)
{
    vkDestroyPipelineLayout(qvk.device, pipeline_layout_dlss_rr, NULL);
    pipeline_layout_dlss_rr = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

VkResult vkpt_dlss_rr_create_pipelines(void)
{
    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = SHADER_STAGE(QVK_MOD_DLSS_RR_PREP_COMP, VK_SHADER_STAGE_COMPUTE_BIT),
        .layout = pipeline_layout_dlss_rr,
    };

    _VK(vkCreateComputePipelines(qvk.device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, pipelines));
    return VK_SUCCESS;
}

VkResult vkpt_dlss_rr_destroy_pipelines(void)
{
    for (int i = 0; i < DLSS_RR_NUM_PIPELINES; ++i) {
        vkDestroyPipeline(qvk.device, pipelines[i], NULL);
        pipelines[i] = VK_NULL_HANDLE;
    }
    return VK_SUCCESS;
}

VkResult vkpt_dlss_rr_prepare_resources(VkCommandBuffer cmd_buf)
{
    VkDescriptorSet desc_sets[] = {
        qvk.desc_set_ubo,
        qvk_get_current_desc_set_textures(),
    };

    vkpt_dlss_vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[DLSS_RR_PREP]);
    vkpt_dlss_vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
        pipeline_layout_dlss_rr, 0, LENGTH(desc_sets), desc_sets, 0, 0);

    vkCmdDispatch(cmd_buf,
        (qvk.extent_screen_images.width + 15) / 16,
        (qvk.extent_screen_images.height + 15) / 16,
        1);

    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_FSR_EASU_OUTPUT]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_FSR_RCAS_OUTPUT]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_HQ_COLOR_INTERLEAVED]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_DLSS_RR_MOTION]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_DLSS_RR_ROUGHNESS]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_DLSS_RR_COLOR]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_DLSS_RR_COLOR_BEFORE_TRANSPARENCY]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_DLSS_RR_DEPTH]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_DLSS_RR_SPEC_HIT_DIST]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_DLSS_RR_SPEC_RAY_DIR_HIT_DIST]);
    BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_DLSS_RR_SPEC_MOTION]);

    return VK_SUCCESS;
}
