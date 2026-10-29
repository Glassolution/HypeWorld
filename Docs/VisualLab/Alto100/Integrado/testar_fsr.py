assert not BENCH_EVENTS
choose_veg('controle')
NAMES.extend(['r.FidelityFX.FSR.QualityMode','r.FidelityFX.FSR.Sharpness','r.FidelityFX.FI.OverrideSwapChainDX12','r.ScreenPercentage','r.DynamicRes.OperationMode'])
def choose_fsr(name):
    cmd('r.FidelityFX.FSR.Enabled 0')
    cmd('r.FidelityFX.FI.Enabled 0')
    cmd('r.FidelityFX.FSR.QualityMode 0')
    cmd('r.FidelityFX.FSR.Sharpness 0')
    cmd('r.ScreenPercentage 100')
    cmd('r.TemporalAA.Upsampling 1')
    cmd('r.AntiAliasingMethod 4')
    cmd('r.FidelityFX.FSR.Enabled '+('1' if name=='fsr_native' else '0'))
    pose(0)
    s=state();assert s['r.ScreenPercentage']=='100';assert s['r.FidelityFX.FI.Enabled']=='0';assert s['r.FidelityFX.FSR.QualityMode']=='0'
    (OUT/(name+'_estado.json')).write_text(json.dumps(s,indent=2))
start_suite(['tsr_q1','fsr_native'],choose_fsr,'fsr')
