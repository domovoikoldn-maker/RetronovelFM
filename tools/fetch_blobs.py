"""Download only the listed files from a blob-less (partial) git clone, in batches."""
import os, subprocess

def materialize(repo_dir, paths):
    paths = sorted(set(paths))
    if not paths: return
    out = subprocess.run(['git', '-C', repo_dir, 'ls-tree', '-r', 'HEAD', '--'] + [], capture_output=True, text=True, check=True).stdout
    oid = {}
    want = set(paths)
    for line in out.split('\n'):
        if not line: continue
        meta, p = line.split('\t', 1)
        if p in want: oid[p] = meta.split()[2]
    missing = [p for p in paths if p not in oid]
    if missing: print('not in tree:', missing[:5])
    todo = [p for p in paths if p in oid and not (os.path.exists(os.path.join(repo_dir, p)) and os.path.getsize(os.path.join(repo_dir, p)) > 0)]
    oids = [oid[p] for p in todo]
    for i in range(0, len(oids), 150):
        subprocess.run(['git', '-C', repo_dir, '-c', 'fetch.negotiationAlgorithm=noop', 'fetch', '-q', 'origin', '--no-tags',
                        '--no-write-fetch-head', '--filter=blob:none'] + oids[i:i + 150], check=False)
    for p in todo:
        dst = os.path.join(repo_dir, p)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        with open(dst, 'wb') as f:
            f.write(subprocess.run(['git', '-C', repo_dir, 'cat-file', 'blob', oid[p]], capture_output=True, check=True).stdout)
    print('%s: %d files ready (%d downloaded)' % (os.path.basename(repo_dir), len(paths), len(todo)))
