#!/usr/bin/python3

import argparse
import csv

def parse_result(line, columns):
    result = {}
    for pair in line.split(' '):
        kv = pair.split('=', 2)
        if len(kv) == 2:
            key = kv[0].strip()
            val = kv[1].strip()
            
            if (not columns) or (key in columns):
                result[key] = val

    return result

# command line
parser = argparse.ArgumentParser(description='RESULT list to CSV')
parser.add_argument('filenames', nargs='+')
parser.add_argument('-o', '--output', required=True)
parser.add_argument('-c', '--columns', nargs='+')
parser.add_argument('-k', '--key')
parser.add_argument('--op', choices=['avg', 'median'])
args = parser.parse_args()

# add key to columns if not already included
if args.columns and args.key and (not args.key in args.columns):
    args.columns = [args.key] + args.columns

# parse input line by line
data = {}

for filename in args.filenames:
    with open(filename, 'r') as f:
        for n, line in enumerate(f):
            if line.startswith('RESULT '):
                r = parse_result(line[7:], args.columns)
                if args.key:
                    key = r[args.key]
                    if not key in data:
                        data[key] = r
                    else:
                        # make lists out of values
                        ds = data[key]
                        for k,v in r.items():
                            if k in ds:
                                if isinstance(ds[k], list):
                                    ds[k] += [v]
                                else:
                                    ds[k] = [ds[k], v]
                            else:
                                ds[k] = v
                        pass
                else:
                    data[len(data)] = r

if args.op == 'avg':
    for key, ds in data.items():
        for k, v in ds.items():
            if isinstance(v, list):
                try:
                    ds[k] = sum(map(lambda x: int(x), v)) / len(v)
                except:
                    ds[k] = v[0]
                    pass
elif args.op == 'median':
    for key, ds in data.items():
        for k, v in ds.items():
            if isinstance(v, list):
                try:
                    ivals = sorted(map(lambda x: int(x), v))
                    if len(ivals) % 2 == 0:
                        med = (ivals[len(ivals)//2] + ivals[len(ivals)//2]) / 2
                    else:
                        med = ivals[len(ivals)//2]

                    ds[k] = med
                except:
                    ds[k] = v[0]
else:
    pass

# export as csv
if args.columns:
    csvcols = args.columns
else:
    colset = set()
    for key, ds in data.items():
        for k, _ in ds.items():
            colset.add(k)
    csvcols = list(colset)

with open(args.output, 'w') as csvfile:
    writer = csv.DictWriter(csvfile, fieldnames=csvcols)
    writer.writeheader()
    for key, ds in data.items():
        writer.writerow(ds)

print(data)
