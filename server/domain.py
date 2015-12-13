from sqlobject import SQLObject, FloatCol, DateTimeCol, connectionForURI, sqlhub

#DATABASE_FILE = '/:memory:'
DATABASE_FILE = '/tmp/laundrydude.db'

class Humidity(SQLObject):
	timestamp = DateTimeCol(default=DateTimeCol.now)
	value = FloatCol()

	@staticmethod
	def getDataType():
		return 'h'

class WasherLed(SQLObject):
	timestamp = DateTimeCol(default=DateTimeCol.now)
	value = FloatCol()

	@staticmethod
	def getDataType():
		return 'l'

def connectDb():
	data_types = {}

	sqlhub.processConnection = connectionForURI('sqlite:%s' % DATABASE_FILE)

	for cls in SQLObject.__subclasses__():
		data_types[cls.getDataType()] = cls

	return data_types

def createDb():
	ret = connectDb()

	Humidity.createTable(True)
	WasherLed.createTable(True)

	return ret
