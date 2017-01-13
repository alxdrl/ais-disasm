#define C6X_UNIT_L1 0
#define C6X_UNIT_L2 1
#define C6X_UNIT_S1 2
#define C6X_UNIT_S2 3
#define C6X_UNIT_M1 4
#define C6X_UNIT_M2 5
#define C6X_UNIT_D1 6
#define C6X_UNIT_D2 7
#define C6X_RES_LD1 8
#define C6X_RES_LD2 9
#define C6X_RES_ST1 10
#define C6X_RES_ST2 11
#define C6X_RES_DA1 12
#define C6X_RES_DA2 13
#define C6X_RES_X1  14
#define C6X_RES_X2  15

#define C6X_UNIT_MASK_L1 (1 << 0)
#define C6X_UNIT_MASK_L2 (1 << 1)
#define C6X_UNIT_MASK_S1 (1 << 2)
#define C6X_UNIT_MASK_S2 (1 << 3)
#define C6X_UNIT_MASK_M1 (1 << 4)
#define C6X_UNIT_MASK_M2 (1 << 5)
#define C6X_UNIT_MASK_D1 (1 << 6)
#define C6X_UNIT_MASK_D2 (1 << 7)
#define C6X_RES_MASK_LD1 (1 << 8)
#define C6X_RES_MASK_LD2 (1 << 9)
#define C6X_RES_MASK_ST1 (1 << 10)
#define C6X_RES_MASK_ST2 (1 << 11)
#define C6X_RES_MASK_DA1 (1 << 12)
#define C6X_RES_MASK_DA2 (1 << 13)
#define C6X_RES_MASK_X1  (1 << 14)
#define C6X_RES_MASK_X2  (1 << 15)

#define C6X_PIPELINE_STAGE_PG 0
#define C6X_PIPELINE_STAGE_PS 1
#define C6X_PIPELINE_STAGE_PW 2
#define C6X_PIPELINE_STAGE_PR 3
#define C6X_PIPELINE_STAGE_DP 4
#define C6X_PIPELINE_STAGE_DC 5
#define C6X_PIPELINE_STAGE_E1 6
#define C6X_PIPELINE_STAGE_E2 7
#define C6X_PIPELINE_STAGE_E3 8
#define C6X_PIPELINE_STAGE_E4 9
#define C6X_PIPELINE_STAGE_E5 10

typedef uint32 c6xreg;

typedef uint32 c6xinsn;

typedef c6xinsn[8] fetch_packet_t;

typedef struct {
	uint8 mask;
	c6xinsn[8] insn;
} exec_packet_t;

typedef struct {
	c6xreg a[32];
	c6xreg b[32];
	c6xreg amr;
	c6xreg csr;
	c6xreg gfpgfr;
	c6xreg icr;
        c6xreg ier;
        c6xreg ifr;
	c6xref irp;
	c6xreg istp;
	c6xreg nrp;
	c6xreg pce1;
} dsp_regs_t;

typedef struct {
	c6xreg ilc;
	c6xreg rilc;
	c6xreg lbc[2];
	fetch_packet_t fetch_packet;
	fetch_packet_t decode_packet;
	exec_packet_t loop_buffer[14];
} dsp_pipeline_t;

typedef struct
{
} l_unit_t;

typedef struct
{
} s_unit_t;

typedef struct
{
} d_unit_t;

typedef struct
{
} m_unit_t;

typedef struct {
	uint64 cycles;
	dsp_regs_t regs;
} dsp_state_t;

typedef struct {
	l_unit_t L1;
	l_unit_t L2;
	s_unit_t S1;
	s_unit_t S2;
	d_unit_t D1;
	d_unit_t D2;
	m_unit_t M1;
	m_unit_t M2;
	dsp_state_t state;
	uint32 bus_addr;
	uint32[8] bus_data
	void * _data_bus_source;
	void * _data_bus_sink;
} dsp_t;

dsp_t c6745;

dsp_fetch_PG(dsp_t *dsp)
{
	dsp->busaddr = dsp->pce1 & ~0x1f;
}

dsp_fetch_PS(dsp_t *dsp)
{
	dsp->_data_bus_source = _bus_mem_source(dsp->bus_addr);
	dsp->_data_busi_sink = _dsp_fetch_sink(dsp)
}

dsp_fetch_PW(dsp_t *dsp)
{
}

dsp_fetch_PR(dsp_t *dsp)
{
	memcpy(_dsp_fetch_source, _dsp_fetch_sink, 8 * 4)
}

dsp_fetch(dsp_t *dsp)
{
	dsp_fetch_PG(dsp);
	dsp_fetch_PS(dsp);
	dsp_fetch_PW(dsp);
	dsp_fetch_PR(dsp);
}

dsp_decode_DP(dsp_t *dsp)
{
	
}

dsp_decode(dsp_t *dsp)
{
	dsp_decode_DP(dsp);
	dsp_decode_DC(dsp);
}

dsp_execute(dsp_t *dsp)
{
	n_unit_exec(dsp);
	l_unit_exec(dsp->L1, dsp->state);
	l_unit_exec(dsp->L2, dsp->state);
	s_unit_exec(dsp->S1, dsp->state);
	s_unit_exec(dsp->S2, dsp->state);
	d_unit_exec(dsp->D1, dsp->state);
	d_unit_exec(dsp->D2, dsp->state);
	d_unit_exec(dsp->M1, dsp->state);
	d_unit_exec(dsp->M2, dsp->state);
}

dsp_run(dsp_t *dsp) {
	while (1) {
		fetch(dsp);
		decode(dsp);
		execute(dsp);
	}
}
