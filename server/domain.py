from sqlobject import SQLObject, FloatCol, IntCol, DateTimeCol, connectionForURI, sqlhub

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

class State(DataPoint):
	value = IntCol()

	@staticmethod
	def getDataType():
		return 's'

def connectDb():
	data_types = {}

	sqlhub.processConnection = connectionForURI('sqlite:%s' % DATABASE_FILE)

	for cls in DataPoint.__subclasses__():
		data_types[cls.getDataType()] = cls

	return data_types

def createDb():
	ret = connectDb()

	for cls in DataPoint.__subclasses__():
		cls.createTable(True)

	return ret
