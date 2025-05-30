# use online calculator to find correct configuration: 
# http://www.bittiming.can-wiki.info/

# from IPython import embed

def ComputeBaudrate(f_fdcan_ker_ck, NBRP, NTSEG1, NTSEG2):
    # From reference manual:
    # RM0399 Rev 4 2639/3556
    # RM0399 Controller area network with flexible data rate (FDCAN)

    t_fdcan_tq_ck = 1.0 / f_fdcan_ker_ck
    t_q = (NBRP + 1) * t_fdcan_tq_ck

    t_SyncSeg = 1 * t_q
    t_BS1 = t_q * (NTSEG1 + 1)
    t_BS2 = t_q * (NTSEG2 + 1)

    t_bit = t_SyncSeg + t_BS1 + t_BS2
    
    print('  t_q: ' + str(t_q * 1E9) + " ns")
    print('  bit time: ' + str(t_bit / t_q) + " t_q")
    print('  baudrate: ' + str(1 / t_bit) + " bit/s")

ComputeBaudrate(f_fdcan_ker_ck=40*1E6, NBRP=0, NTSEG1=30, NTSEG2=7)
ComputeBaudrate(f_fdcan_ker_ck=40*1E6, NBRP=3, NTSEG1=7, NTSEG2=0)
ComputeBaudrate(f_fdcan_ker_ck=40*1E6, NBRP=4, NTSEG1=5, NTSEG2=0)

print('250 kbit/s sample point at 87,5 %')
ComputeBaudrate(f_fdcan_ker_ck=40*1E6, NBRP=1-1, NTSEG1=139-1, NTSEG2=20-1)

print('500 kbit/s sample point at 87,5 %')
ComputeBaudrate(f_fdcan_ker_ck=40*1E6, NBRP=1-1, NTSEG1=69-1, NTSEG2=10-1)

print('250 kbit/s sample point at 75 %')
ComputeBaudrate(f_fdcan_ker_ck=40*1E6, NBRP=1-1, NTSEG1=119-1, NTSEG2=40-1)

print('500 kbit/s sample point at 75 %')
ComputeBaudrate(f_fdcan_ker_ck=40*1E6, NBRP=1-1, NTSEG1=59-1, NTSEG2=20-1)

print('1 Mbit/s sample point at 75 %')
ComputeBaudrate(f_fdcan_ker_ck=40*1E6, NBRP=1-1, NTSEG1=29-1, NTSEG2=10-1)

