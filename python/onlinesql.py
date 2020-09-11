import mysql.connector

class mySequel:
    def connectToDB():
        mydb = mysql.connector.connect(
            host = "hostname", # This is the domain where the database is held
            user = "username",
            password = "password",
            database = "database_name",
            # port = 3306
            # auth_plugin = "mysql_native_password"
        )
        return mydb

    def addLine(database, cursor, data_array):
        query = "INSERT IGNORE INTO garden_data (unix_time, date, soil_temp, soil_moisture, air_temp, air_humidity, heat_index, sunlight, watered_today) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)"
        values = (
            data_array[0],
            data_array[1],
            data_array[2],
            data_array[3],
            data_array[4],
            data_array[5],
            data_array[6],
            data_array[7],
            data_array[8]
        )
        database.ping(True)
        cursor.execute(query, values)
        database.ping(True)
        database.commit() # This still doesn't work for gardenBTSerialReader, but it worked in prompt!



# """ Creating the table""" # "UNIQUE" statement after unix_time makes sure two duplicate entries are not added
# cursor.execute("CREATE TABLE garden_data (unix_time VARCHAR(10) UNIQUE, date VARCHAR(25), soil_temp VARCHAR(6), soil_moisture VARCHAR(6), air_temp VARCHAR(6), air_humidity VARCHAR(6), heat_index VARCHAR(6), sunlight VARCHAR(6), watered_today VARCHAR(6))")



# Example query add:
# query = "INSERT IGNORE INTO garden_data (unix_time, date, soil_temp, soil_moisture, air_temp, air_humidity, heat_index, sunlight, watered_today) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)"
# values = ("1597777777","8/3/2020 3:53:58 PM","27.19","0.36","26.5","60.4","27.5","73.63","0")
# cursor.execute(query, values)


# Required to make changes
# db.commit()

# cursor.execute("SELECT * FROM garden_data")
# records = cursor.fetchall()
# for record in records:
#     print(record)
