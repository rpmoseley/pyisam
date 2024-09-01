'''
This module provides some example definitions that match the files used by the
commercial product Strategix/OneOffice sold by KCS Commercial Systems (originally
TIS Software Ltd).
'''

from . import CharColumn, TextColumn, ShortColumn, LongColumn
from . import DuplicateIndex, PrimaryIndex, UniqueIndex

__all__ = ('AppcodeColumn', 'AppdescColumn',
           'DECOMPdefn', 'DEITEMdefn', 'DEFILEdefn', 'DEKEYSdefn',
           'DEBFILEdefn', 'DEBKEYSdefn', 'DEBCOMPdefn')

# Provide a variant of the CharColumn that adds the support for appcode specific to the 
# Strategix/OneOffice product
class AppcodeColumn(CharColumn):
  __slots__ = ('_appcode',)
  def __init__(self, name, appcode=None, **kwd):
    super().__init__(name, **kwd)
    if appcode is not None:
      self._appcode = appcode

# Provide a variant of the CharColumn that adds the support of appdesc specific to the
# Strategix/OneOffice product
class AppdescColumn(CharColumn):
  __slots__ = ('_appdesc',)
  def __init__(self, name, appdesc=None, **kwd):
    super().__init__(self, name, **kwd)
    if appdesc is not None:
      self._appdesc = appdesc

# Examples of the usage of ISAMtable and ISAMtableDefn
class DECOMPdefn:
  _tabname = 'decomp'
  _columns = (TextColumn('comp',9),
              CharColumn('comptyp'),
              TextColumn('sys',9),
              TextColumn('prefix',5),
              TextColumn('user',4),
              TextColumn('database',6),
              TextColumn('release',5),
              LongColumn('timeup'),
              LongColumn('specup'))
  _indexes = (PrimaryIndex('comp'),
              DuplicateIndex('prefix'),
              UniqueIndex('typkey','comptyp','comp'),
              UniqueIndex('syskey','sys','comptyp','comp'))
  _prefix = 'dec'
  _database = 'utool'

class DEITEMdefn:
  _tabname = 'deitem'
  _columns = (TextColumn('item',9),
              ShortColumn('seq'),
              CharColumn('comptyp'),
              TextColumn('comp',9),
              CharColumn('spec'))
  _indexes = (PrimaryIndex('key','item','seq','comptyp','comp'),
              UniqueIndex('usekey','comp','item'),
              UniqueIndex('compkey','item','comptyp','comp','seq'))
  _prefix = 'deit'
  _database = 'utool'

class DEFILEdefn:
  _tabname = 'defile'
  _columns = (TextColumn('filename',9),
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
  _indexes = (PrimaryIndex('key','filename','seq'),
              UniqueIndex('unikey','filename','field'),
              UniqueIndex('vkey','filename','vseq','field'))
  _prefix = 'def'
  _database = 'utool'

class DEKEYSdefn:
  _tabname = 'dekeys'
  _columns = (TextColumn('filename',9),
              TextColumn('keyfield',9),
              TextColumn('key1',9),
              TextColumn('key2',9),
              TextColumn('key3',9),
              TextColumn('key4',9),
              TextColumn('key5',9),
              TextColumn('key6',9),
              TextColumn('key7',9),
              TextColumn('key8',9))
  _indexes = PrimaryIndex('key','filename','keyfield')
  _prefix = 'dek'
  _database = 'utool'

class DEBFILEdefn:
  _tabname = 'debfile'
  _columns = (CharColumn('source'),
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
  _indexes = (PrimaryIndex('key','source','license','filename','dataset','field'),
              DuplicateIndex('fkey','filename','dataset','field'))
  _prefix = 'debf'
  _database = 'utool'

class DEBKEYSdefn:
  _tabname = 'debkeys'
  _columns = (CharColumn('source'),
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
  _indexes = PrimaryIndex('key','source','license','dataset','filename','keyfield')
  _prefix = 'debk'
  _database = 'utool'

class DEBCOMPdefn:
  _tabname = 'debcomp'
  _columns = (TextColumn('filename',9),
              CharColumn('stxbuild'),
              TextColumn('mask',5),
              TextColumn('dataset',5),
              TextColumn('location',128))
  _indexes = (PrimaryIndex('filename'),
              DuplicateIndex('mask'))
  _prefix = 'debc'
  _database = 'utool'
