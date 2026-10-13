"""Run generated corpus on isolated MySQL/MariaDB test servers (PyMySQL).

Refuses non-loopback connections and data directories without the dedicated
DbSchemaStudio-091 marker. Uses fresh dbschema_verify_* databases only.
"""
import argparse
import json
import uuid
from pathlib import Path
import pymysql

parser = argparse.ArgumentParser()
parser.add_argument('corpus', type=Path)
parser.add_argument('--port', type=int, required=True)
parser.add_argument('--profile', choices=['mysql-8.4', 'mariadb-10.11'], required=True)
parser.add_argument('--case', help='Run one corpus case by name')
parser.add_argument('--expected-datadir', type=Path, help='Require the exact isolated server directory')
args = parser.parse_args()
db = pymysql.connect(host='127.0.0.1', port=args.port, user='root', charset='utf8mb4', autocommit=True)
cur = db.cursor()
cur.execute('SELECT VERSION(), @@datadir, @@innodb_page_size')
version, datadir, pagesize = cur.fetchone()
assert ('DbSchemaStudio-091-' in datadir or 'DbSchemaStudio-beta1-' in datadir), f'Not a dedicated test server: {datadir}'
if args.expected_datadir:
    assert Path(datadir).resolve() == args.expected_datadir.resolve(), f'Unexpected data directory: {datadir}'
assert (version.startswith('8.4.') if args.profile == 'mysql-8.4' else '10.11.' in version)
assert pagesize == 16384
print(f'{args.profile}: {version}; InnoDB page size {pagesize}', flush=True)
checks = 0
def check(ok, label):
    global checks
    checks += 1
    if not ok:
        raise AssertionError(label)

def query(sql, params=()):
    cur.execute(sql, params)
    return cur.fetchall()

def q(name):
    return '`' + name.replace('`', '``') + '`'

files = sorted(args.corpus.glob(args.profile + '-*.json'))
if args.case:
    files = [p for p in files if p.stem == args.profile + '-' + args.case]
assert files
for path in files:
    case = json.loads(path.read_text(encoding='utf-8'))
    name = 'dbschema_verify_' + uuid.uuid4().hex[:16]
    create_container = any(stmt.startswith('CREATE DATABASE ') for stmt in case['statements'])
    if not create_container:
        cur.execute('CREATE DATABASE ' + q(name) + ' CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci')
        cur.execute('USE ' + q(name))
    try:
        for stmt in case['statements']:
            try:
                cur.execute(stmt.replace('`dbschema_test`', q(name)))
            except pymysql.MySQLError:
                print('FAILED DDL', case['name'], stmt, flush=True)
                raise
        cur.execute('USE ' + q(name))
        for table in case['manifest']['tables']:
            tn = table['name']
            meta = query('SELECT COLUMN_NAME,DATA_TYPE,IS_NULLABLE,CHARACTER_MAXIMUM_LENGTH,NUMERIC_PRECISION,NUMERIC_SCALE,EXTRA FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA=%s AND TABLE_NAME=%s ORDER BY ORDINAL_POSITION', (name, tn))
            check(len(meta) == len(table['columns']), tn + ' column count')
            for spec, actual in zip(table['columns'], meta):
                cn, typ, nullable, length, precision, scale, extra = actual
                expected = {'INTEGER': 'int', 'BOOLEAN': 'tinyint'}.get(spec['type'], spec['type'].lower())
                check(cn == spec['name'] and typ == expected and (nullable == 'YES') == spec['nullable'], str((tn, spec, actual)))
                if expected in ('varchar', 'char'):
                    check(length == spec['length'], tn + '.' + cn + ' length')
                if expected == 'decimal':
                    check((precision, scale) == (spec['precision'], spec['scale']), tn + '.' + cn + ' precision')
                check(('auto_increment' in extra) == spec['identity'], tn + '.' + cn + ' identity')
            pk = query("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE WHERE TABLE_SCHEMA=%s AND TABLE_NAME=%s AND CONSTRAINT_NAME='PRIMARY' ORDER BY ORDINAL_POSITION", (name, tn))
            check([v[0] for v in pk] == table['primaryKey'], tn + ' PK')
            actual_fks = query('SELECT k.CONSTRAINT_NAME,k.COLUMN_NAME,k.REFERENCED_TABLE_NAME,k.REFERENCED_COLUMN_NAME,r.DELETE_RULE,r.UPDATE_RULE FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE k JOIN INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS r ON k.CONSTRAINT_SCHEMA=r.CONSTRAINT_SCHEMA AND k.CONSTRAINT_NAME=r.CONSTRAINT_NAME AND k.TABLE_NAME=r.TABLE_NAME WHERE k.TABLE_SCHEMA=%s AND k.TABLE_NAME=%s ORDER BY k.CONSTRAINT_NAME,k.ORDINAL_POSITION', (name, tn))
            groups = {}
            for constraint, col, target, target_col, delete, update in actual_fks:
                groups.setdefault(constraint, []).append((col, target.lower(), target_col, delete.replace('RESTRICT', 'NO ACTION'), update.replace('RESTRICT', 'NO ACTION')))
            expected_fks = [[(a, fk['target'].lower(), b, fk['delete'], fk['update']) for a, b in zip(fk['columns'], fk['targetColumns'])] for fk in table['foreignKeys']]
            check(sorted(groups.values()) == sorted(expected_fks), tn + ' FK metadata ' + repr((groups, expected_fks)))
            unique_rows = query("SELECT INDEX_NAME,COLUMN_NAME FROM INFORMATION_SCHEMA.STATISTICS WHERE TABLE_SCHEMA=%s AND TABLE_NAME=%s AND NON_UNIQUE=0 AND INDEX_NAME<>'PRIMARY' ORDER BY INDEX_NAME,SEQ_IN_INDEX", (name, tn))
            groups = {}
            for index, col in unique_rows:
                groups.setdefault(index, []).append(col)
            check(sorted(groups.values()) == sorted(table['unique']), tn + ' UNIQUE')
            engine = query('SELECT ENGINE,TABLE_COLLATION FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA=%s AND TABLE_NAME=%s', (name, tn))[0]
            check(engine == ('InnoDB', 'utf8mb4_unicode_ci'), tn + ' engine/collation')
        for cmd in case['execute']:
            try:
                cur.execute(cmd['sql'])
            except pymysql.MySQLError as exc:
                # A syntax or unknown-column error must never count as a rejected constraint.
                check(cmd['reject'] and exc.args[0] in (1048, 1062, 1264, 1406, 1451, 1452, 3819, 4025), str((cmd, exc)))
            else:
                check(not cmd['reject'], 'Expected rejection: ' + cmd['sql'])
        if case['name'] == 'money':
            amounts = query('SELECT Quantita FROM RigaOrdine ORDER BY NumeroRiga')
            check([str(row[0]) for row in amounts] == ['123.4567', '-999999999999999.9999', '-12.3456'], 'Money four-digit precision, full range and default preserved')
        if case['name'] == 'ordini':
            check(query('SELECT NumeroOrdine FROM Ordine') == ((10,),), 'identity initial value')
        if case['name'] in ('types-and-constraints', 'referential-actions'):
            values = query('SELECT Amount,Text,Flag,Date,Time,Stamp FROM Child WHERE Id=1')[0]
            check(str(values[0]) == '-12.30' and values[1] == "O'Brien; -- testo" and values[2] == 1 and str(values[3]) == '2026-09-20' and values[5] is not None, 'literal/current defaults')
            try:
                cur.execute('INSERT INTO Child (Id,Flag) VALUES (50,2)')
            except pymysql.MySQLError as exc:
                check(exc.args[0] in (3819, 4025), 'boolean CHECK rejects 2')
            else:
                check(False, 'boolean CHECK missing')
        if case['name'] == 'current-date-time':
            check(all(v is not None for v in query('SELECT Date,Time,Stamp FROM Child WHERE Id=1')[0]), 'current date/time defaults')
        if case['name'] == 'unicode-text':
            check(query('SELECT Text FROM Child WHERE Id=1')[0][0] == "C:\\percorso\\O'Brien; -- caffè 名 😀", 'UTF8 apostrophe and backslash preserved')
        if case['name'] == 'single-CASCADE':
            check(query('SELECT COUNT(*) FROM Child') == ((0,),), 'CASCADE deletes child')
        if case['name'] == 'single-SET NULL':
            check(query('SELECT ParentA FROM Child') == ((None,),), 'SET NULL updates child')
            cur.execute("INSERT INTO Parent VALUES (20,'P20','CODE20')")
            cur.execute('INSERT INTO Child (Id,ParentA) VALUES (20,20)')
            cur.execute('DELETE FROM Parent WHERE A=20')
            check(query('SELECT ParentA FROM Child WHERE Id=20') == ((None,),), 'SET NULL deletes parent, retains child')
        if case['name'] == 'referential-actions':
            check(query('SELECT ParentA,ParentB FROM Child WHERE Id=1') == ((None, None),), 'SET NULL on composite FK')
        print('PASS', case['name'], flush=True)
    finally:
        # name was generated here; datadir checked before any destructive SQL.
        cur.execute('DROP DATABASE ' + q(name))
print(f'PASS {len(files)} cases; {checks} checks', flush=True)
db.close()
