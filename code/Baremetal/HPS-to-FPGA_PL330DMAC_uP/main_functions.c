/*
 * main_functions.h
 *
 *  Created on: 30/3/2016
 *      Author: Roberto Fernandez Molanes
 *      Explanation: Main functions used in the main() file. Putr here just to clean-up the other file.
 *
 */
//
#include <stdio.h>
#include <string.h> //to use memcpy
#include <inttypes.h>

#include "alt_address_space.h"
#include "alt_dma_modified.h"
#include "socal/alt_acpidmap.h"
#include "socal/socal.h"

//--------------------------DMA FUNCTIONS--------------------------//
ALT_STATUS_CODE system_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: Setting up DMA.\n\r");

    // Uninit DMA
    if(status == ALT_E_SUCCESS)
    {
        status = alt_dma_uninit();
    }

    // Configure everything as defaults.
    if (status == ALT_E_SUCCESS)
    {
        ALT_DMA_CFG_t dma_config;
        dma_config.manager_sec = ALT_DMA_SECURITY_DEFAULT;
        for (int i = 0; i < 8; ++i)
        {
            dma_config.irq_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (int i = 0; i < 32; ++i)
        {
            dma_config.periph_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (int i = 0; i < 4; ++i)
        {
            dma_config.periph_mux[i] = ALT_DMA_PERIPH_MUX_DEFAULT;
        }

        status = alt_dma_init(&dma_config);
    }


    return status;
}

ALT_STATUS_CODE system_uninit(void)
{
    printf("INFO: System shutdown.\n\r");
    printf("\n\r");
    return ALT_E_SUCCESS;
}
