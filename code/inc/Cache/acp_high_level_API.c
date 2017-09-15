#define soc_cv_av
#include "alt_address_space.h" //to use ACP ID mapper functions
#include <stdio.h> //printf

//---------------------------ACP CONFIGURATION---------------------------//
/*
This configures the ACP ID mapper to dinamically translate the AXI ID from any
incoming master into ID 3 in reads and ID 4 for writes for the L2Controller.
Therefore the L2C sees: ID 0 when CPU0 accesses, ID 1 when CPU1 accesses,
ID 3 when any master in ACP reads, ID 4 when any master writes in ACP.
*/
ALT_STATUS_CODE acp_configuration(void)
{
	printf("\n\rConfiguring ACP\n\r");
	//printf("Before acp config VID3RD_S:%#x, VID4WR_S:%#x, DYNRD:%#x, DYNWR:%#x\n\r", *VID3RD_S, *VID4WR_S, *DYNRD, *DYNWR);
	const uint32_t ARUSER = 0b11111; //coherent cacheable reads
	const uint32_t AWUSER = 0b11111; //coherent cacheable writes
	ALT_STATUS_CODE status = ALT_E_SUCCESS;

	//Set output ID3 for dynamic reads and ID4 for dynamic writes
	status = alt_acp_id_map_dynamic_read_set(ALT_ACP_ID_OUT_DYNAM_ID_3);
	if (status != ALT_E_SUCCESS)
	{
		printf ("Error when configuring ID3 as dynamic for reads\n\r"); return status;
	}
	status = alt_acp_id_map_dynamic_write_set(ALT_ACP_ID_OUT_DYNAM_ID_4);
	if (status != ALT_E_SUCCESS)
	{
		printf ("Error when configuring ID4 as dynamic for writes\n\r"); return status;
	}
	//Configure the page and user write sideband signal options that are applied
	//to all write transactions that have their input IDs dynamically mapped.
	status = alt_acp_id_map_dynamic_read_options_set(ALT_ACP_ID_MAP_PAGE_0, ARUSER);
	if (status != ALT_E_SUCCESS)
	{
		printf ("Error when setting options for dynamic reads\n\r"); return status;
	}
	status = alt_acp_id_map_dynamic_write_options_set(ALT_ACP_ID_MAP_PAGE_0, AWUSER);
	if (status != ALT_E_SUCCESS)
	{
		printf ("Error when setting options for dynamic writes\n\r"); return status;
	}
	//printf("After acp config VID3RD_S:%#x, VID4WR_S:%#x, DYNRD:%#x, DYNWR:%#x\n\r", *VID3RD_S, *VID4WR_S, *DYNRD, *DYNWR);
	return status;
}
