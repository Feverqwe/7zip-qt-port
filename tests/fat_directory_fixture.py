"""FAT namespace failures with readable siblings and independently mapped children."""
from fat_fixture import Volume, PAYLOAD, LONG_NAME, entry, long_entries

MODES = ('control', 'free', 'range', 'bad', 'cycle', 'start', 'eoc', 'alias',
         'iochild', 'ioroot', 'prefix', 'eodprefix', 'root', 'emptyroot')


def directory_volume(kind):
    bits, mode = int(kind[-2:]), kind[:-2]
    if bits not in (12, 16, 32) or mode not in MODES:
        raise ValueError(kind)
    v = Volume(bits, sectors_per_cluster=4)
    files = {LONG_NAME: PAYLOAD, 'EMPTY.TXT': b'', 'GOOD/GOOD.TXT': b'healthy sibling\n'}
    v.set_fat(6, v.eof)
    v.data[v.offset(6):v.offset(6)+len(files['GOOD/GOOD.TXT'])] = files['GOOD/GOOD.TXT']
    v.records += [entry(b'BROKEN     ', 10, attr=0x10), entry(b'GOOD       ', 12, attr=0x10)]
    if mode == 'alias':
        v.records.append(entry(b'ALIAS      ', 10, attr=0x10))

    def directory(cluster, parent, records, full=False):
        v.set_fat(cluster, v.eof)
        deleted = bytes([0xe5]) + bytes(31)
        rows = [entry(b'.          ', cluster, attr=0x10),
                entry(b'..         ', parent, attr=0x10)] + records
        content = b''.join(rows)
        content += (deleted * (v.cluster_size // 32 - len(rows))) if full else bytes(v.cluster_size-len(content))
        v.data[v.offset(cluster):v.offset(cluster)+v.cluster_size] = content

    directory(12, 0 if bits != 32 else 2,
              [entry(b'GOOD    TXT', 6, len(files['GOOD/GOOD.TXT']))])
    files['BROKEN/PREFIX.TXT'] = b'kept prefix\n'
    files['BROKEN/NEST/INNER.BIN'] = b'inner payload\n'
    for cluster, name in ((8, 'BROKEN/PREFIX.TXT'), (18, 'BROKEN/NEST/INNER.BIN')):
        v.set_fat(cluster, v.eof)
        v.data[v.offset(cluster):v.offset(cluster)+len(files[name])] = files[name]
    directory(16, 10, [entry(b'INNER   BIN', 18, len(files['BROKEN/NEST/INNER.BIN']))])
    directory(10, 0 if bits != 32 else 2,
              [entry(b'PREFIX  TXT', 8, len(files['BROKEN/PREFIX.TXT'])),
               entry(b'NEST       ', 16, attr=0x10)], full=True)
    if mode in ('free', 'range', 'bad', 'cycle'):
        value = {'free': 0, 'range': v.clusters+10, 'bad': v.eof-8, 'cycle': 10}[mode]
        v.set_fat(10, value)
    if mode in ('start', 'eoc'):
        v.records[-2] = entry(b'BROKEN     ', 0 if mode == 'start' else v.eof, attr=0x10)
        files.pop('BROKEN/PREFIX.TXT')
        files.pop('BROKEN/NEST/INNER.BIN')
    if mode in ('iochild', 'ioroot', 'prefix', 'eodprefix'):
        target = v.root if mode == 'ioroot' else v.offset(10)
        alias = b'LATE    TXT' if mode == 'ioroot' else b'TARGET  TXT'
        late_name = 'LATE.TXT' if mode == 'ioroot' else 'BROKEN/TARGET.TXT'
        v.set_fat(20, v.eof)
        payload = b'later sector\n'
        v.data[v.offset(20):v.offset(20)+len(payload)] = payload
        files[late_name] = payload
        # Crossing an unreadable sector must discard pending LFN state.
        lfn = long_entries('untrusted long name.txt', alias)
        deleted = bytes([0xe5]) + bytes(31)
        if mode == 'ioroot':
            v.records += [deleted] * (15-len(v.records)) + [lfn[0], lfn[1]]
            v.records += [deleted] * (32-len(v.records)) + [lfn[1], entry(alias, 20, len(payload))]
        else:
            v.data[target+15*32:target+17*32] = lfn[0] + lfn[1]
            v.data[target+32*32:target+34*32] = lfn[1] + entry(alias, 20, len(payload))
            if mode == 'eodprefix':
                v.data[target+16*32:target+v.cluster_size] = bytes(v.cluster_size-16*32)
                files.pop(late_name)
        v.fault_offset = target + 512
    if mode in ('root', 'emptyroot'):
        if bits != 32:
            raise ValueError('root chain requires FAT32')
        v.set_fat(2, 0)
        if mode == 'emptyroot':
            v.records.clear()
            files.clear()
    v.write_directory()
    if mode in ('root', 'emptyroot'):
        start = v.root + len(v.records) * 32
        v.data[start:v.root+v.cluster_size] = (bytes([0xe5]) + bytes(31)) * ((v.root+v.cluster_size-start)//32)
    if mode == 'alias':
        for name, data in list(files.items()):
            if name.startswith('BROKEN/'):
                files['ALIAS/' + name[len('BROKEN/'):]] = data
    v.expected_files = files
    v.incomplete_dirs = ['BROKEN'] if mode in ('free','range','bad','cycle','start','eoc','iochild','prefix') else []
    if mode in ('ioroot', 'root', 'emptyroot'):v.directory_limited = True
    else:v.directory_limited = bool(v.incomplete_dirs)
    if mode == 'eodprefix':v.directory_limited = False
    return v
