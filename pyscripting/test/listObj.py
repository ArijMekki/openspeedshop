# <ListOf_filename> = listObj [ <expId_spec> ] [ <target> ]

import openss

my_viewtype = openss.ViewTypeList("pcsamp")
my_file = openss.FileList("../../usability/phaseIII/fred")

my_id	= openss.expCreate(my_file,my_viewtype)
#my_host = openss.HostList(["host1,host2"])

ret = openss.listObj()

#output = openss.listObj(my_id)
#output = openss.listObj(my_host)
#output = openss.listObj(my_id,my_host)

print " "
print ret
print " "

r_count = len(ret)
for row_ndx in range(r_count):
   print ret[row_ndx]

openss.exit()
