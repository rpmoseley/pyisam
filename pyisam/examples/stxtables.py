'''
This module provides some example definitions that match the files used by the
commercial product Strategix/OneOffice sold by KCS Commercial Systems.
'''

from ..table import ISAMtableDefn,TextColumn,CharColumn,ShortColumn,LongColumn

# Examples of the usage of ISAMtable and ISAMtableDefn
class DECOMPdefn(ISAMtableDefn):
  _columns_ = (TextColumn('comp',9),
               CharColumn('comptyp'),
               TextColumn('sys',9),
               TextColumn('prefix',5),
               TextColumn('user',4),
               TextColumn('database',6),
               TextColumn('release',5),
               LongColumn('timeup'),
               LongColumn('specup'))
  _indexes_ = (('comp',False),
               ('prefix',True),
               ('typkey',False,'comptyp','comp'),
               ('syskey',False,'sys','comptyp','comp'))
  _prefix_ = 'dec'
  _database_ = 'utool'

class DEITEMdefn(ISAMtableDefn):
  _columns_ = (TextColumn('item',9),
               ShortColumn('seq'),
               CharColumn('comptyp'),
               TextColumn('comp',9),
               CharColumn('spec'))
  _indexes_ = (('key',False,'item','seq','comptyp','comp'),
               ('usekey',False,'comp','item'),
               ('compkey',False,'item','comptyp','comp','seq'))
  _prefix_ = 'deit'
  _database_ = 'utool'

class DEFILEdefn(ISAMtableDefn):
  _columns_ = (TextColumn('filename',9),
               ShortColumn('seq'),
               TextColumn('field',9),
               TextColumn('refptr',9),
               CharColumn('type'),
               ShortColumn('size'),
               CharColumn('keytype'),
               ShortColumn('vseq'),
               ShortColumn('stype'),
               CharColumn('scode'),
               TextColumn('fgroup',10),
               CharColumn('idxflag'))
  _indexes_ = (('key',False,'filename','seq'),
               ('unikey',False,'filename','field'),
               ('vkey',False,'filename','vseq','field'))
  _prefix_ = 'def'
  _database_ = 'utool'

class DEKEYSdefn(ISAMtableDefn):
  _columns_ = (TextColumn('filename',9),
               TextColumn('keyfield',9),
               TextColumn('key1',9),
               TextColumn('key2',9),
               TextColumn('key3',9),
               TextColumn('key4',9),
               TextColumn('key5',9),
               TextColumn('key6',9),
               TextColumn('key7',9),
               TextColumn('key8',9))
  _indexes_ = (('key',False,'filename','keyfield'),)
  _prefix_ = 'dek'
  _database_ = 'utool'

class DEBFILEdefn(ISAMtableDefn):
  _columns_ = (CharColumn('source'),
               LongColumn('license'),
               TextColumn('filename',9),
               TextColumn('dataset',4),
               TextColumn('field',9),
               CharColumn('action'),
               ShortColumn('stype'),
               ShortColumn('size'),
               CharColumn('scode'),
               CharColumn('keytype'),
               ShortColumn('vseq'),
               ShortColumn('seq'),
               TextColumn('group',10),
               TextColumn('refptr',9))
  _indexes_ = (('key',False,'source','license','filename','dataset','field'),
               ('fkey',True,'filename','dataset','field'))
  _prefix_ = 'debf'
  _database_ = 'utool'

class DEBKEYSdefn(ISAMtableDefn):
  _columns_ = (CharColumn('source'),
               LongColumn('license'),
               TextColumn('dataset',4),
               TextColumn('filename',9),
               TextColumn('keyfield',9),
               TextColumn('key1',9),
               TextColumn('key2',9),
               TextColumn('key3',9),
               TextColumn('key4',9),
               TextColumn('key5',9),
               TextColumn('key6',9),
               TextColumn('key7',9),
               TextColumn('key8',9))
  _indexes_ = (('key',False,'source','license','dataset','filename','keyfield'),)
  _prefix_ = 'debk'
  _database_ = 'utool'

class DEBCOMPdefn(ISAMtableDefn):
  _columns_ = (TextColumn('filename',9),
               CharColumn('stxbuild'),
               TextColumn('mask',5),
               TextColumn('dataset',5),
               TextColumn('location',128))
  _indexes_ = (('filename',False),
               ('mask',True))
  _prefix_ = 'debc'
  _database_ = 'utool'
