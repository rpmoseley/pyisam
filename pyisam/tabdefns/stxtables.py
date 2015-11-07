'''
This module provides some example definitions that match the files used by the
commercial product Strategix/OneOffice sold by KCS Commercial Systems (originally
TIS Software Ltd).
'''

from . import *

__all__ = ('AppcodeColumnDef','AppdescColumnDef',
           'DECOMPdefn','DEITEMdefn','DEFILEdefn','DEKEYSdefn',
           'DEBFILEdefn','DEBKEYSdefn','DEBCOMPdefn')

# Provide a variant of the CharColumn that adds the support for appcode specific to the 
# Strategix/OneOffice product
class AppcodeColumnDef(CharColumnDef):
  __slots__ = ('_appcode_',)
  def __init__(self, name, appcode=None):
    super().__init__(name)
    if appcode is not None:
      self._appcode_ = appcode

# Provide a variant of the CharColumn that adds the support of appdesc specific to the
# Strategix/OneOffice product
class AppdescColumnDef(CharColumnDef):
  __slots__ = ('_appdesc_',)
  def __init__(self, name, appdesc=None):
    super().__init__(name)
    if appdesc is not None:
      self._appdesc_ = appdesc

# Examples of the usage of ISAMtable and ISAMtableDefn
class DECOMPdefn:
  _tabname_ = 'decomp'
  _columns_ = (TextColumnDef('comp',9),
               CharColumnDef('comptyp'),
               TextColumnDef('sys',9),
               TextColumnDef('prefix',5),
               TextColumnDef('user',4),
               TextColumnDef('database',6),
               TextColumnDef('release',5),
               LongColumnDef('timeup'),
               LongColumnDef('specup'))
  _indexes_ = (PrimaryIndexDef('comp'),
               DuplicateIndexDef('prefix'),
               UniqueIndexDef('typkey','comptyp','comp'),
               UniqueIndexDef('syskey','sys','comptyp','comp'))
  _prefix_ = 'dec'
  _database_ = 'utool'

class DEITEMdefn:
  _tabname_ = 'deitem'
  _columns_ = (TextColumnDef('item',9),
               ShortColumnDef('seq'),
               CharColumnDef('comptyp'),
               TextColumnDef('comp',9),
               CharColumnDef('spec'))
  _indexes_ = (PrimaryIndexDef('key','item','seq','comptyp','comp'),
               UniqueIndexDef('usekey','comp','item'),
               UniqueIndexDef('compkey','item','comptyp','comp','seq'))
  _prefix_ = 'deit'
  _database_ = 'utool'

class DEFILEdefn:
  _tabname_ = 'defile'
  _columns_ = (TextColumnDef('filename',9),
               ShortColumnDef('seq'),
               TextColumnDef('field',9),
               TextColumnDef('refptr',9),
               CharColumnDef('type'),
               ShortColumnDef('size'),
               CharColumnDef('keytype'),
               ShortColumnDef('vseq'),
               ShortColumnDef('stype'),
               CharColumnDef('scode'),
               TextColumnDef('fgroup',10),
               CharColumnDef('idxflag'))
  _indexes_ = (PrimaryIndexDef('key','filename','seq'),
               UniqueIndexDef('unikey','filename','field'),
               UniqueIndexDef('vkey','filename','vseq','field'))
  _prefix_ = 'def'
  _database_ = 'utool'

class DEKEYSdefn:
  _tabname_ = 'dekeys'
  _columns_ = (TextColumnDef('filename',9),
               TextColumnDef('keyfield',9),
               TextColumnDef('key1',9),
               TextColumnDef('key2',9),
               TextColumnDef('key3',9),
               TextColumnDef('key4',9),
               TextColumnDef('key5',9),
               TextColumnDef('key6',9),
               TextColumnDef('key7',9),
               TextColumnDef('key8',9))
  _indexes_ = PrimaryIndexDef('key','filename','keyfield')
  _prefix_ = 'dek'
  _database_ = 'utool'

class DEBFILEdefn:
  _tabname_ = 'debfile'
  _columns_ = (CharColumnDef('source'),
               LongColumnDef('license'),
               TextColumnDef('filename',9),
               TextColumnDef('dataset',4),
               TextColumnDef('field',9),
               CharColumnDef('action'),
               ShortColumnDef('stype'),
               ShortColumnDef('size'),
               CharColumnDef('scode'),
               CharColumnDef('keytype'),
               ShortColumnDef('vseq'),
               ShortColumnDef('seq'),
               TextColumnDef('group',10),
               TextColumnDef('refptr',9))
  _indexes_ = (PrimaryIndexDef('key','source','license','filename','dataset','field'),
               DuplicateIndexDef('fkey','filename','dataset','field'))
  _prefix_ = 'debf'
  _database_ = 'utool'

class DEBKEYSdefn:
  _tabname_ = 'debkeys'
  _columns_ = (CharColumnDef('source'),
               LongColumnDef('license'),
               TextColumnDef('dataset',4),
               TextColumnDef('filename',9),
               TextColumnDef('keyfield',9),
               TextColumnDef('key1',9),
               TextColumnDef('key2',9),
               TextColumnDef('key3',9),
               TextColumnDef('key4',9),
               TextColumnDef('key5',9),
               TextColumnDef('key6',9),
               TextColumnDef('key7',9),
               TextColumnDef('key8',9))
  _indexes_ = PrimaryIndexDef('key','source','license','dataset','filename','keyfield')
  _prefix_ = 'debk'
  _database_ = 'utool'

class DEBCOMPdefn:
  _tabname_ = 'debcomp'
  _columns_ = (TextColumnDef('filename',9),
               CharColumnDef('stxbuild'),
               TextColumnDef('mask',5),
               TextColumnDef('dataset',5),
               TextColumnDef('location',128))
  _indexes_ = (PrimaryIndexDef('filename'),
               DuplicateIndexDef('mask'))
  _prefix_ = 'debc'
  _database_ = 'utool'
