from pathlib import Path
import csv,json,sys
import numpy as np
for name in sys.argv[1:]:
    home=Path(__file__).parent/name;m=json.loads((home/'result.json').read_text());begin,end=[x['qpc'] for x in m['events']]
    rows=[r for r in csv.DictReader((home/'presentmon.csv').open()) if r.get('TimeInQPC') and r.get('MsUntilDisplayed') not in (None,'','NA') and begin+1e7<int(r['TimeInQPC'])<end-1e7]
    present=np.array([int(r['TimeInQPC']) for r in rows])/1e4
    display=present+np.array([float(r['MsUntilDisplayed']) for r in rows])
    d=np.diff(np.sort(display));backwards=int((np.diff(display)<-.001).sum())
    result=dict(name=name,frames=len(rows),present_order_differs_from_display_order=backwards,sorted_display_intervals_ms=np.quantile(d,[0,.01,.1,.5,.9,.99,1]).tolist(),below1ms_percent=float((d<1).mean()*100),above8ms_percent=float((d>8).mean()*100),mean_ms=float(d.mean()))
    print(json.dumps(result,indent=2));(home/'display_order.json').write_text(json.dumps(result,indent=2))
