'''
This module provides some example definitions that match the files used by the
commercial product Strategix/OneOffice sold by KCS Commercial Systems (originally
TIS Software Ltd).
'''

from ..table import *

__all__ = ('AppcodeColumn','AppdescColumn',
           'DECOMPdefn','DEITEMdefn','DEFILEdefn','DEKEYSdefn',
           'DEBFILEdefn','DEBKEYSdefn','DEBCOMPdefn')

# Provide a variant of the CharColumn that adds the support for appcode specific to the 
# Strategix/OneOffice product
class AppcodeColumn(CharColumn):
  __slots__ = ('_appcode_',)
  def __init__(self, name, appcode=None):
    super().__init__(name)
    if appcode is not None:
      self._appcode_ = appcode

# Provide a variant of the CharColumn that adds the support of appdesc specific to the
# Strategix/OneOffice product
class AppdescColumn(CharColumn):
  __slots__ = ('_appdesc_',)
  def __init__(self, name, appdesc=None):
    super().__init__(name)
    if appdesc is not None:
      self._appdesc_ = appdesc

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
  _indexes_ = (PrimaryKey('comp'),
               DuplicateKey('prefix'),
               UniqueKey('typkey','comptyp','comp'),
               UniqueKey('syskey','sys','comptyp','comp'))
  _prefix_ = 'dec'
  _database_ = 'utool'

class DEITEMdefn(ISAMtableDefn):
  _columns_ = (TextColumn('item',9),
               ShortColumn('seq'),
               CharColumn('comptyp'),
               TextColumn('comp',9),
               CharColumn('spec'))
  _indexes_ = (PrimaryKey('key','item','seq','comptyp','comp'),
               UniqueKey('usekey','comp','item'),
               UniqueKey('compkey','item','comptyp','comp','seq'))
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
  _indexes_ = (PrimaryKey('key','filename','seq'),
               UniqueKey('unikey','filename','field'),
               UniqueKey('vkey','filename','vseq','field'))
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
  _indexes_ = PrimaryKey('key','filename','keyfield')
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
  _indexes_ = (PrimaryKey('key','source','license','filename','dataset','field'),
               DuplicateKey('fkey','filename','dataset','field'))
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
  _indexes_ = PrimaryKey('key','source','license','dataset','filename','keyfield')
  _prefix_ = 'debk'
  _database_ = 'utool'

class DEBCOMPdefn(ISAMtableDefn):
  _columns_ = (TextColumn('filename',9),
               CharColumn('stxbuild'),
               TextColumn('mask',5),
               TextColumn('dataset',5),
               TextColumn('location',128))
  _indexes_ = (PrimaryKey('filename'),
               DuplicateKey('mask'))
  _prefix_ = 'debc'
  _database_ = 'utool'