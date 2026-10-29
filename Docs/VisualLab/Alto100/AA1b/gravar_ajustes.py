assert not EVENTS and (OUT/'benchmark_concluido.json').exists()
exec('def vtick'+(OUT/'diagnosticar.py').read_text().split('def vtick',1)[1].split('\nrecord(DIAG)')[0])
# Each clip repeats exactly the original 10-second route at native 1080p.
record(TUNING[1:],300)
