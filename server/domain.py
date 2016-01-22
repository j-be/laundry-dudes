from sqlobject import SQLObject, FloatCol, IntCol, DateTimeCol, connectionForURI, sqlhub, UnicodeCol
from sqlobject.sresults import SelectResults


#DATABASE_FILE = '/:memory:'
DATABASE_FILE = '/tmp/laundrydude.db'

class DataPoint(SQLObject):
	timestamp = DateTimeCol(default=DateTimeCol.now)

class Humidity(DataPoint):
	value = FloatCol()

	@staticmethod
	def getDataType():
		return 'h'

class Temperature(DataPoint):
	value = FloatCol()

	@staticmethod
	def getDataType():
		return 't'

class WasherLed(DataPoint):
	value = FloatCol()

	@staticmethod
	def getDataType():
		return 'l'

class WasherAccel(DataPoint):
	value = FloatCol()

	@staticmethod
	def getDataType():
		return 'a'

class State(DataPoint):
	value = IntCol()

	@staticmethod
	def getDataType():
		return 's'

class RfidCard(DataPoint):
	value = UnicodeCol()

	@staticmethod
	def getDataType():
		return 'r'

class Reservation(SQLObject):
	start = DateTimeCol()
	user = UnicodeCol()

class User(SQLObject):
	name = UnicodeCol()
	email = UnicodeCol()
	rfid = UnicodeCol()

def connectDb():
	data_types = {}

	sqlhub.processConnection = connectionForURI('sqlite:%s' % DATABASE_FILE)

	for cls in DataPoint.__subclasses__():
		data_types[cls.getDataType()] = cls

	return data_types

def createDb():
	ret = connectDb()

	for cls in SQLObject.__subclasses__():
		cls.createTable(True)
	for cls in DataPoint.__subclasses__():
		cls.createTable(True)

	User.deleteMany("1 = 1")
	User(name="Blue", rfid="01005D0EB9EB", email="")
	User(name="Red", rfid="01000543581F", email="")

	return ret

def sqlresultToDict(sqlObject, recursive_scan = False):
	''' Converts a SQLObject to a Dictionary containing only its columns

	Keyword arguments:
	sqlObject -- the SQLObject
	recursive_scan -- if True, ForeignKeys will be resolved to their respective values;
	                  else, only the ForeignKey-ID is added
	'''
	sqlObject_type = type(sqlObject)

	if (isinstance(sqlObject, SelectResults)):
		json_dict = []
		for item in sqlObject:
			json_dict.append(sqlresultToDict(item))
	elif (isinstance(sqlObject, SQLObject)):
		json_dict = {}
		json_dict['id'] = sqlObject.id
		for attr in vars(sqlObject_type):
			if isinstance(getattr(sqlObject_type, attr), property):
				attr_value = getattr(sqlObject, attr)
				attr_parent = type(attr_value).__bases__[0]
				if attr_parent == SQLObject:
					if recursive_scan:
						json_dict[attr] = sqlresultToDict(attr_value)
				elif type(attr_value) == SelectResults:
					json_dict[attr] = sqlresultToDict(attr_value)
				else:
					if (not isinstance(attr_value, list)):
						json_dict[attr] = attr_value
					else:
						json_dict[attr] = []
						for item in attr_value:
							## TODO: Try to improve
							json_dict[attr].append(item.id)
	else:
		# DEBUG
		print sqlObject
		json_dict = {}

	return json_dict

def sqlresultToDictList(sqlObject, recursive_scan = True):
	return {'list':sqlresultToDict(sqlObject, recursive_scan)}
