#!/usr/bin/python3
import os
import sys
from datetime import datetime
import zipfile
def kml_head(kml_project_name):
    return """<?xml version="1.0" encoding="UTF-8"?><kml xmlns="http://www.opengis.net/kml/2.2" xmlns:gx="http://www.google.com/kml/ext/2.2" xmlns:kml="http://www.opengis.net/kml/2.2" xmlns:atom="http://www.w3.org/2005/Atom"><Document><name>{}</name>
    <Placemark>
<gx:Track>""".format(kml_project_name)

def kml_cord(datetime,longitude,latitude,altitude):
   return """<when>{}</when>
<gx:coord>{} {} {}</gx:coord>""".format(datetime,longitude,latitude,altitude)

def kml_eof():
    return """</gx:Track>
</Placemark></Document></kml>"""
def datetime_processor(csv_datetime):
    #datetime.strptime(csv_datetime, "%y%d%m_%H%M%S")
    datetime_obj=datetime.strptime(csv_datetime, "%Y-%m-%d_%H:%M:%S.%f")
    return datetime_obj.strftime('%Y-%m-%dT%H:%M:%S.%f%Z')
def main(csv_input,kml_filename):
    filename_wo_ext=os.path.splitext(csv_input)[0]
    data=""
    data=kml_head(filename_wo_ext)+'\n'
    #print(kml_head("test"))
    with open(csv_input,"r") as gps_csv:
        for line in gps_csv.readlines()[1:]:
           gps=line.strip().split(',')
           #print(gps)
           data=data+kml_cord(datetime_processor(gps[0]+"_"+gps[1]),gps[3],gps[2],gps[4])+'\n'
           #print(data)
           #print(gps[4])
    #print(kml_eof())
    data=data+kml_eof()+'\n'
#main("small_test_data.csv","test2.kml")
    #with open(kml_filename,'w') as kml_file:
    #    kml_file.write(data)
    kmz_filename=filename_wo_ext+'.kmz'
    kml_filename=filename_wo_ext+'.kml'
    with zipfile.ZipFile(kmz_filename,  mode='w',compresslevel=zipfile.ZIP_DEFLATED) as kmz_file:
        kmz_file.writestr(kml_filename,data,zipfile.ZIP_DEFLATED)
def autoname(filename):
    filename_wo_ext=os.path.splitext(filename)[0]
    #print(filename_wo_ext)
    main(filename_wo_ext+".csv",filename_wo_ext+".kml")
if (len(sys.argv)==2) and (os.path.exists(sys.argv[1])):    autoname(sys.argv[1])