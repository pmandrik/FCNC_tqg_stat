# -*- coding: utf-8 -*-

"""
/////////////////////////////////////////////////////////////////////////////
  Author      :     P.Mandrik, IHEP
  Date        :     02/09/17
  Version     :     0.5.0
/////////////////////////////////////////////////////////////////////////////

  Python module TxtDatabase.py provide class for write and read
  access into simple human readeble txt database:

    order : key : comment : list of data as single string with commas
    order : key : comment : list of data as single string with commas
    order : key : comment : list of data as single string with commas
    
/////////////////////////////////////////////////////////////////////////////
  Changelog : 
    02/09/17 v0.5.0
    days of creation, version 0.5.0
"""

class TxtDatabase():
  def __init__(self, file_name):
    self.fname = file_name;
    self.data  = {}
    self.next_order = 0

  def Read(self):
    ifile         = open(self.fname, "r")
    raw_data = ifile.read()

    for line in raw_data.split("\n"):
      if not line or len( line ) < 3 : continue
      sublines = line.split(":")
      try:
        order = int(sublines[0])
        self.next_order = max(order, self.next_order)
        key = sublines[1].strip()
        comment = sublines[2].strip()
        value = sublines[3].strip()
        # print str(order)+"|"+key+"|"+comment+"|"+value
        self.data[key] = [value,comment, order]
      except : continue

  def Write(self):
    ofile = open(self.fname, "w")

    data_list = self.data.items()
    data_list.sort(key=lambda x: x[1][2], reverse=False) # sort by order
    for item in data_list:
      ofile.write( str(item[1][2]) + " : " + str(item[0]) + " : " + str(item[1][1]) + " : " );
      if type(item[1][0]) == type(list()):
        db = [str(val) for val in item[1][0] ]
        ofile.write( " ".join(db) + " \n" );
      else : ofile.write( str(item[1][0]) + " \n" );
    ofile.close()
  
  def GetItem(self, key):
    answer = None
    try:
      answer = self.data.get(key)[0]
      if answer : return answer
    except: pass
    if not answer : print "TxtDatabase.GetItem():", self.fname, "wrong key = ", "\"" + key + "\""
    return None

  def GetComment(self, key):
    answer = None
    try:
      answer = self.data.get(key)[1]
      if answer : return answer
    except: pass
    if not answer : print "TxtDatabase.GetComment():", self.fname, "wrong key = ", "\"" + key + "\""
    return None

  def SetItem(self, key, value, comment=""):
    item = self.data.get(key)
    if item:
      order = item[2]
      self.data[key] = [value, comment, order]
      return
    order = self.next_order
    self.next_order += 1
    self.data[key] = [value, comment, order]
      

def test():
  print "TxtDatabase() usage test ... "

  print "write ... "
  db = TxtDatabase("tmp.txt");
  db.SetItem("a", 12313, "test()")
  db.SetItem("b", "123 132 123 123 123 123", "test()")
  db.SetItem("abc", [123, 1334, 551, 515123], "test()")
  db.Write()
  print db.data

  print "read ... "
  db.Read()
  print db.data

  print "read and write ... "
  db.Read()
  db.Write()
  print db.data

if __name__ == "__main__": 
  import argparse
  parser = argparse.ArgumentParser(description='Produce some theta .cfg datacards ... ')
  parser.add_argument('--file',  dest='file',    type=str, default='db.txt', help='database name')
  parser.add_argument('--key',   dest='key',    type=str, default='',       help='key to get'   )
  args = parser.parse_args()
  if args.key:
    db = TxtDatabase(args.file) 
    db.Read()
    print db.GetItem(args.key)
  else : test()



