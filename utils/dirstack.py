'''
This module provides a stack of diretories in the ilk of the builtin shell pushd/popd/dirs commands
expect that in the first version directores are stored as absolute paths rather than be altered to
be use the tilde character for a users' home directory (just like the '-l' option).
'''

import os

class PathStack:
  '''Provide a stack of directories which can be used in a 'with' statement to change
     directory within the block before returning to the previous directory'''
  _dirinfo = ([], [])   # (DIRSTK, DIRCTX)
  
  def pushd(self, newdir):
    'Push the current directory and change to the given NEWDIR'
    if os.path.isdir(newdir):
      self._dirinfo[0].append(os.getcwd())
      os.chdir(newdir)
    else:
      raise ValueError('Attempt to switch to non-existant directory')
    return self
      
  def popd(self):
    'Pop and change directory to the last entry in the stack'
    os.chdir(self._dirinfo[0].pop())

  def __enter__(self):
    'Remember the length of the directory stack so that on exit we unwind correctly'
    self._dirinfo[1].append(len(self._dirinfo[0]))
    return self

  def __exit__(self, *args):
    'Reverse back up the directories until the length matches the one saved on entry'
    dirlen = self._dirinfo[1].pop()
    while len(self._dirinfo[0]) > dirlen:
      self._dirinfo[0].pop()
    os.chdir(self._dirinfo[0].pop())


def test_PathStack():
  dirstk = PathStack()
  origcwd = os.getcwd()
  with dirstk.pushd('/tmp'):
    dirstk.pushd('/usr/bin')
    os.chdir('/usr/lib')
    dirstk.popd()
  assert(os.getcwd() == origcwd)
  assert(len(dirstk._dirinfo[0]) == 0 and len(dirstk._dirinfo[1]) == 0)
  with PathStack().pushd('/tmp') as dirstk2:
    with dirstk2.pushd('/usr/lib'):
      print(dirstk2._dirinfo[0], os.getcwd())
    dirstk2.pushd('/usr/bin')
  assert(os.getcwd() == origcwd)
  assert(len(dirstk2._dirinfo[0]) == 0 and len(dirstk2._dirinfo[1]) == 0)

if __name__ == '__main__':
  test_PathStack()
