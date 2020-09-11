import mysql.connector

class mySequel:
    def connectToDB():
        mydb = mysql.connector.connect(
            host = "hostname", # This is the domain where the database is held
            user = "username",
            password = "password",
            database = "database_name",
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
        database.commit()
