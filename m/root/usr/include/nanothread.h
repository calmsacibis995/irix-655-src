#ifndef _NANOTHREAD_H
#define _NANOTHREAD_H

#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)

#define _KMEMUSER
#include <sys/types.h>
#include <sys/reg.h>
#include <sys/kusharena.h>

/*      Ids of nanothreads       */
typedef short nid_t;

/*      Thread entry points are of this type.  */
typedef void ((*nanostartf_t) (void *));

typedef int (*resumefptr_t) (nid_t, kusharena_t *, nid_t);
typedef int (*runptr_t) (resumefptr_t, kusharena_t *, nid_t);
extern int _run_nid (resumefptr_t, kusharena_t *, nid_t);

/*
 *    upcall_handler_t(upcall_type, bad_nid, bad_rsaid, reserved,
 *                     arg4, arg5, arg6, arg7):
 *              Handler interface expected for OS upcalls.  arg? is
 *              set by the user through setup_upcall()
 */
typedef void ((*upcall_handler_t) (greg_t, greg_t, greg_t, greg_t,
				   greg_t, greg_t, greg_t, greg_t));

/*
 *    set_num_processors(nr, static_rsa_alloc):
 *              Set number of requested processors to nr.
 *              Allocate kernel-user shared arena if needed.
 */
int set_num_processors (nid_t, int);

/*
 *    setup_upcall(upcall_handler, arg4, arg5, arg6, arg7):
 *              Sets registers that will be passed to upcall routine
 *              specified.  First four args are used by the OS to
 *              communicate to the handler.  See sys/kusharena.h
 *              details.
 */
void setup_upcall (upcall_handler_t, greg_t, greg_t, greg_t, greg_t);

/*
 *    start_threads(init_upcall):
 *              Start 'nrequested' threads running init_upcall.
 */
int start_threads (nanostartf_t);
void kill_threads (int signal);
void wait_threads (void);

/*
 *    resume_nid(myid, kusp, resumeid): Give-up processor of the caller
 *              and resume resumeid.
 */
int resume_nid (nid_t myid, kusharena_t * kusp, nid_t resume);
/*
 * Specific paths of resume that can be invoked directly.
 */
int resume_nid_static (nid_t myid, kusharena_t * kusp, nid_t resume);
int resume_nid_dynamic (nid_t myid, kusharena_t * kusp, nid_t resume);
int heavy_resume_nid (nid_t myid, kusharena_t * kusp, nid_t resume);
/*
 *      run_nid(nidresume, kusp, newnid): A long jump to a nanothread.
 *             Also path of resume_nid that may be invoked directly.
 *             The supplied nidresume pointer is used to detect
 *             nested resumes.  If the applications uses the
 *             resume_nid supplied in this library no other resume
 *             may be used.
 */
void run_nid (resumefptr_t nidresume, kusharena_t * kusp, nid_t newnid);

/*
 *    getnid(): Return id of calling nanothread.
 */
nid_t getnid (void);
nid_t getnid_ (void);

/*
 *    block_nid(v), unblock_nid(v): Suspend or resume execution.
 */
void block_nid (nid_t);
void unblock_nid (nid_t);

__inline uint64_t
set_bit (volatile uint64_t * bitv, nid_t nid)
{
	return (__fetch_and_or (&bitv[nid / 64], 1LL < (nid % 64)) & ~(1LL << (nid % 64)));
}

__inline uint64_t
unset_bit (volatile uint64_t * bitv, nid_t nid)
{
	return (__fetch_and_and (&bitv[nid / 64], ~(1LL << (nid % 64))) & (1LL << (nid % 64)));
}

__inline nid_t
unset_any_bit (volatile uint64_t * bitv, int approx_length)
{
	register uint64_t cmask;
	register nid_t offset = 0;
	register uint8_t delta, location;
	while (approx_length > offset) {
		cmask = *bitv;
		while (cmask != 0) {
			for (location = 32, delta = 16; delta > 0; delta = delta / 2) {
				if ((cmask >> location) != 0) {
					location += delta;
				}
				else {
					location += -delta;
				}
			}
			if ((cmask >> location) == 0) {
				location--;
			}
			cmask = __fetch_and_and (bitv, ~(1LL << location));
			if (cmask & (1LL << location)) {
				return (location + offset);
			}
		}
		offset += 64;
		bitv++;
	}
	return (NULL_NID);
}

/*
 *    kusp: Pointer to kernel-user shared arena,
 *              accessible to all nanothreads/execution vehicles.
 *              Setup by set_num_processors().
 */
extern kusharena_t *kusp;

/* Don't touch the line below.  It's used for string matching. */
#endif /* defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS) */

/*
 * Job state
 */
#define NANO_processors_set    0x1
#define NANO_upcall_set        0x2
#define NANO_signaling         0x4	/* signal to all ev's */
#define NANO_exiting           0x8

#if defined(_LANGUAGE_ASSEMBLY)
#if (_MIPS_SIM == _ABIN32)
#define PRDA 0x200000
#define PRDA_NID 0xe4c
#define PRDA_UNUSED0 0xe54
#define PRDA_UNUSED1 0xe58
#define PRDA_UNUSED2 0xe5c
#define PRDA_UNUSED3 0xe60
#define PRDA_UNUSED4 0xe64
#define PRDA_UNUSED5 0xe68
#define KUS_RSA 0x980
#define KUS_NREQUESTED 0x0
#define KUS_NALLOCATED 0x4
#define KUS_RBITS 0x80
#define KUS_FBITS 0x900
#define KUS_NIDTORSA 0x100
#define KUS_PAD 0x8
#define RSA_SIZE 560
#define PADDED_RSA_SIZE 640
#define NT_MAXNIDS 1024
#define NT_MAXRSAS 1024
#define NULL_NID 0
#define RSA_R0 0
#define RSA_A0 32
#define RSA_A1 40
#define RSA_A2 48
#define RSA_A3 56
#define RSA_AT 8
#define RSA_FP 240
#define RSA_GP 224
#define RSA_RA 248
#define RSA_S0 128
#define RSA_S1 136
#define RSA_S2 144
#define RSA_S3 152
#define RSA_S4 160
#define RSA_S5 168
#define RSA_S6 176
#define RSA_S7 184
#define RSA_SP 232
#define RSA_A4 64
#define RSA_A5 72
#define RSA_A6 80
#define RSA_A7 88
#define RSA_T0 96
#define RSA_T1 104
#define RSA_T2 112
#define RSA_T3 120
#define RSA_T8 192
#define RSA_T9 200
#define RSA_V0 16
#define RSA_V1 24
#define RSA_MDHI 264
#define RSA_MDLO 256
#define RSA_EPC 280
#define RSA_FV0 296
#define RSA_FV1 312
#define RSA_FA0 392
#define RSA_FA1 400
#define RSA_FA2 408
#define RSA_FA3 416
#define RSA_FA4 424
#define RSA_FA5 432
#define RSA_FA6 440
#define RSA_FA7 448
#define RSA_FT0 328
#define RSA_FT1 336
#define RSA_FT2 344
#define RSA_FT3 352
#define RSA_FT4 360
#define RSA_FT5 368
#define RSA_FT6 376
#define RSA_FT7 384
#define RSA_FT8 464
#define RSA_FT9 480
#define RSA_FT10 496
#define RSA_FT11 512
#define RSA_FT12 528
#define RSA_FT13 544
#define RSA_FT14 304
#define RSA_FT15 320
#define RSA_FS0 456
#define RSA_FS1 472
#define RSA_FS2 488
#define RSA_FS3 504
#define RSA_FS4 520
#define RSA_FS5 536
#define RSA_FPC_CSR 552
#endif /* (_MIPS_SIM == _ABIN32) */
#if (_MIPS_SIM == _ABI64)
#define PRDA 0x200000
#define PRDA_NID 0xe4c
#define PRDA_UNUSED0 0xe54
#define PRDA_UNUSED1 0xe58
#define PRDA_UNUSED2 0xe5c
#define PRDA_UNUSED3 0xe60
#define PRDA_UNUSED4 0xe64
#define PRDA_UNUSED5 0xe68
#define KUS_RSA 0x980
#define KUS_NREQUESTED 0x0
#define KUS_NALLOCATED 0x4
#define KUS_RBITS 0x80
#define KUS_FBITS 0x900
#define KUS_NIDTORSA 0x100
#define KUS_PAD 0x8
#define RSA_SIZE 560
#define PADDED_RSA_SIZE 640
#define NT_MAXNIDS 1024
#define NT_MAXRSAS 1024
#define NULL_NID 0
#define RSA_R0 0
#define RSA_A0 32
#define RSA_A1 40
#define RSA_A2 48
#define RSA_A3 56
#define RSA_AT 8
#define RSA_FP 240
#define RSA_GP 224
#define RSA_RA 248
#define RSA_S0 128
#define RSA_S1 136
#define RSA_S2 144
#define RSA_S3 152
#define RSA_S4 160
#define RSA_S5 168
#define RSA_S6 176
#define RSA_S7 184
#define RSA_SP 232
#define RSA_A4 64
#define RSA_A5 72
#define RSA_A6 80
#define RSA_A7 88
#define RSA_T0 96
#define RSA_T1 104
#define RSA_T2 112
#define RSA_T3 120
#define RSA_T8 192
#define RSA_T9 200
#define RSA_V0 16
#define RSA_V1 24
#define RSA_MDHI 264
#define RSA_MDLO 256
#define RSA_EPC 280
#define RSA_FV0 296
#define RSA_FV1 312
#define RSA_FA0 392
#define RSA_FA1 400
#define RSA_FA2 408
#define RSA_FA3 416
#define RSA_FA4 424
#define RSA_FA5 432
#define RSA_FA6 440
#define RSA_FA7 448
#define RSA_FT0 328
#define RSA_FT1 336
#define RSA_FT2 344
#define RSA_FT3 352
#define RSA_FT4 360
#define RSA_FT5 368
#define RSA_FT6 376
#define RSA_FT7 384
#define RSA_FT8 456
#define RSA_FT9 464
#define RSA_FT10 472
#define RSA_FT11 480
#define RSA_FT12 304
#define RSA_FT13 320
#define RSA_FS0 488
#define RSA_FS1 496
#define RSA_FS2 504
#define RSA_FS3 512
#define RSA_FS4 520
#define RSA_FS5 528
#define RSA_FS6 536
#define RSA_FS7 544
#define RSA_FPC_CSR 552
#endif /* (_MIPS_SIM == _ABI64) */
#if (_MIPS_SIM == _ABIN32)
#define RSA_SAVE_SREGS(contextp, tempreg) \
	REG_S		s0, KUS_RSA+RSA_S0(contextp); \
	REG_S		s1, KUS_RSA+RSA_S1(contextp); \
	REG_S		s2, KUS_RSA+RSA_S2(contextp); \
	REG_S		s3, KUS_RSA+RSA_S3(contextp); \
	REG_S		s4, KUS_RSA+RSA_S4(contextp); \
	REG_S		s5, KUS_RSA+RSA_S5(contextp); \
	REG_S		s6, KUS_RSA+RSA_S6(contextp); \
	REG_S		s7, KUS_RSA+RSA_S7(contextp); \
	REG_S		gp, KUS_RSA+RSA_GP(contextp); \
	REG_S		sp, KUS_RSA+RSA_SP(contextp); \
	REG_S		fp, KUS_RSA+RSA_FP(contextp); \
	REG_S		ra, KUS_RSA+RSA_EPC(contextp); \
	sdc1		fs0, KUS_RSA+RSA_FS0(contextp); \
	sdc1		fs1, KUS_RSA+RSA_FS1(contextp); \
	sdc1		fs2, KUS_RSA+RSA_FS2(contextp); \
	sdc1		fs3, KUS_RSA+RSA_FS3(contextp); \
	sdc1		fs4, KUS_RSA+RSA_FS4(contextp); \
	sdc1		fs5, KUS_RSA+RSA_FS5(contextp); \
	cfc1		tempreg, fpc_csr;
#endif
#if (_MIPS_SIM == _ABI64)
#define RSA_SAVE_SREGS(contextp, tempreg) \
	REG_S		s0, KUS_RSA+RSA_S0(contextp); \
	REG_S		s1, KUS_RSA+RSA_S1(contextp); \
	REG_S		s2, KUS_RSA+RSA_S2(contextp); \
	REG_S		s3, KUS_RSA+RSA_S3(contextp); \
	REG_S		s4, KUS_RSA+RSA_S4(contextp); \
	REG_S		s5, KUS_RSA+RSA_S5(contextp); \
	REG_S		s6, KUS_RSA+RSA_S6(contextp); \
	REG_S		s7, KUS_RSA+RSA_S7(contextp); \
	REG_S		gp, KUS_RSA+RSA_GP(contextp); \
	REG_S		sp, KUS_RSA+RSA_SP(contextp); \
	REG_S		fp, KUS_RSA+RSA_FP(contextp); \
	REG_S		ra, KUS_RSA+RSA_EPC(contextp); \
	sdc1		fs0, KUS_RSA+RSA_FS0(contextp); \
	sdc1		fs1, KUS_RSA+RSA_FS1(contextp); \
	sdc1		fs2, KUS_RSA+RSA_FS2(contextp); \
	sdc1		fs3, KUS_RSA+RSA_FS3(contextp); \
	sdc1		fs4, KUS_RSA+RSA_FS4(contextp); \
	sdc1		fs5, KUS_RSA+RSA_FS5(contextp); \
	sdc1		fs6, KUS_RSA+RSA_FS6(contextp); \
	sdc1		fs7, KUS_RSA+RSA_FS7(contextp); \
	cfc1		tempreg, fpc_csr;
#endif

#if (_MIPS_SIM == _ABIN32)
#define RSA_LOAD_SREGS(contextp, tempreg) \
	REG_L		s0, KUS_RSA+RSA_S0(contextp); \
	REG_L		s1, KUS_RSA+RSA_S1(contextp); \
	REG_L		s2, KUS_RSA+RSA_S2(contextp); \
	REG_L		s3, KUS_RSA+RSA_S3(contextp); \
	REG_L		s4, KUS_RSA+RSA_S4(contextp); \
	REG_L		s5, KUS_RSA+RSA_S5(contextp); \
	REG_L		s6, KUS_RSA+RSA_S6(contextp); \
	REG_L		s7, KUS_RSA+RSA_S7(contextp); \
	REG_L		gp, KUS_RSA+RSA_GP(contextp); \
	REG_L		sp, KUS_RSA+RSA_SP(contextp); \
	REG_L		fp, KUS_RSA+RSA_FP(contextp); \
	REG_L		ra, KUS_RSA+RSA_EPC(contextp); \
	ldc1		fs0, KUS_RSA+RSA_FS0(contextp); \
	ldc1		fs1, KUS_RSA+RSA_FS1(contextp); \
	ldc1		fs2, KUS_RSA+RSA_FS2(contextp); \
	ldc1		fs3, KUS_RSA+RSA_FS3(contextp); \
	ldc1		fs4, KUS_RSA+RSA_FS4(contextp); \
	ldc1		fs5, KUS_RSA+RSA_FS5(contextp); \
	lw		tempreg, KUS_RSA+RSA_FPC_CSR(contextp);
#endif
#if (_MIPS_SIM == _ABI64)
#define RSA_LOAD_SREGS(contextp, tempreg) \
	REG_L		s0, KUS_RSA+RSA_S0(contextp); \
	REG_L		s1, KUS_RSA+RSA_S1(contextp); \
	REG_L		s2, KUS_RSA+RSA_S2(contextp); \
	REG_L		s3, KUS_RSA+RSA_S3(contextp); \
	REG_L		s4, KUS_RSA+RSA_S4(contextp); \
	REG_L		s5, KUS_RSA+RSA_S5(contextp); \
	REG_L		s6, KUS_RSA+RSA_S6(contextp); \
	REG_L		s7, KUS_RSA+RSA_S7(contextp); \
	REG_L		gp, KUS_RSA+RSA_GP(contextp); \
	REG_L		sp, KUS_RSA+RSA_SP(contextp); \
	REG_L		fp, KUS_RSA+RSA_FP(contextp); \
	REG_L		ra, KUS_RSA+RSA_EPC(contextp); \
	ldc1		fs0, KUS_RSA+RSA_FS0(contextp); \
	ldc1		fs1, KUS_RSA+RSA_FS1(contextp); \
	ldc1		fs2, KUS_RSA+RSA_FS2(contextp); \
	ldc1		fs3, KUS_RSA+RSA_FS3(contextp); \
	ldc1		fs4, KUS_RSA+RSA_FS4(contextp); \
	ldc1		fs5, KUS_RSA+RSA_FS5(contextp); \
	ldc1		fs6, KUS_RSA+RSA_FS6(contextp); \
	ldc1		fs7, KUS_RSA+RSA_FS7(contextp); \
	lw		tempreg, KUS_RSA+RSA_FPC_CSR(contextp);
#endif
/* Don't touch the line below.  It's used for string matching. */
#endif /* defined(_LANGUAGE_ASSEMBLY) */

#endif /* _NANOTHREAD_H */
