import csv,json,sys,collections
from pathlib import Path
import numpy as np
for name in sys.argv[1:]:
    home=Path(__file__).parent/name;m=json.loads((home/'result.json').read_text());begin,end=[x['qpc'] for x in m['events']];freq=m['qpc_frequency']
    rows=[r for r in csv.DictReader((home/'presentmon.csv').open()) if r.get('TimeInQPC') and r.get('MsClickToPhotonLatency') and begin+freq<int(r['TimeInQPC'])<end-freq]
    def stats(key):
        v=np.array([float(r[key]) for r in rows if r[key] not in ('NA','')]);v=v[np.isfinite(v)&(v>=0)&(v<10000)]
        positive=v[v>0]
        return dict(n=len(v),zeros=int((v==0).sum()),avg=float(v.mean()),quantiles=np.quantile(positive,[0,.01,.1,.5,.9,.99,1]).tolist()) if len(positive) else None
    result=dict(name=name,rows=len(rows),seconds=(end-begin)/freq-2,modes=dict(collections.Counter(r['PresentMode'] for r in rows)),frame_types=dict(collections.Counter(r['FrameType'] for r in rows)),metrics={k:stats(k) for k in ['MsBetweenPresents','MsBetweenDisplayChange','MsUntilDisplayed','MsAnimationError','MsFlipDelay']})
    print(json.dumps(result,indent=2));(home/'pm_analysis.json').write_text(json.dumps(result,indent=2))
