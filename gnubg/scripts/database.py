#
# database.py
#
# by Joern Thyssen <jth@gnubg.org>, 2004
#
# This file contains the functions for adding matches to a relational
# database.
#
# The modules use the DB API V2 python modules. 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id$
#
                                                                                  

import gnubg
import sys
import os
import traceback
import string

# different database types
DB_SQLITE = 1
DB_POSTGRESQL = 2

# Change this line to set the database type
DB_TYPE = DB_SQLITE

class relational:

   def __init__(self):

      self.conn = None

   #
   # Return next free "id" from the given table
   # Paramters: conn (the database connection, DB API v2)
   #            tablename (the tablename)
   # Returns: next free "id"
   #

   def __next_id(self,tablename):

      # fetch next_id from control table

      q = "SELECT next_id FROM control WHERE tablename = '%s'" % (tablename)

      c = self.conn.cursor()
      c.execute(q)
      row = c.fetchone()
      c.close()

      if row:
     
         next_id = row[0]

         # update control data with new next id
         
         q = "UPDATE control SET next_id = %d WHERE tablename = '%s'" \
             % (next_id+1, tablename)
         
         c = self.conn.cursor()
         c.execute(q)
         c.close()
     
      else:
         next_id = 0

         # insert next id

         q = "INSERT INTO control (tablename,next_id) VALUES ('%s',%d)" \
             % (tablename,next_id+1)
         
         c = self.conn.cursor()
         c.execute(q)
         c.close()
      
      return next_id

   # check if player exists
   def __playerExists(self,name,env_id):
      
      query = "SELECT person_id FROM nick WHERE name = '%s' AND env_id = %d" % \
         ( name, env_id )

      cursor = self.conn.cursor()
      cursor.execute(query)
      row = cursor.fetchone()
      cursor.close()

      if row:
         return row[0]
      else:
         return -1

   #
   # Add players to database
   #
   # Parameters: conn (the database connection, DB API v2)
   #             name (the name of the player)
   #             env_id (the env. of the match)
   #
   # Returns: player id or None on error
   #
   
   def __addPlayer(self,name,env_id):
      
      # check if player exists
      person_id = self.__playerExists(name, env_id)
      if (person_id == -1):
         # player does not exist - Insert new player
         person_id = self.__next_id("person")

         # first add a new person
         query = ("INSERT INTO person(person_id,name,notes)" \
            "VALUES (%d,'%s','')") % (person_id, name)
         cursor = self.conn.cursor()
         cursor.execute(query)
         cursor.close()
         # now add the nickname
         query = ("INSERT INTO nick(env_id,person_id,name)" \
            "VALUES (%d,'%d','%s')") % (env_id, person_id, name)
         cursor = self.conn.cursor()
         cursor.execute(query)
         cursor.close()
         
      return person_id

   #
   # getKey: return key if found in dict, otherwise return empty string
   #
   #

   def __getKey(self,d,key):
      
      if d.has_key(key):
         return d[key]
      else:
         return ''

   #
   # Calculate a/b, but return 0 for b=0
   #

   def __calc_rate(self,a,b):
      if b == 0:
         return 0.0
      else:
         return a/b

   #
   # Add statistics
   # Parameters: conn (the database connection, DB API v2)
   #             person_id (the player id for this statistics)
   #             gs (the game/match statistics)
   #
   
   def __addStat(self,match_id,person_id,gs,gs_other):
      
      matchstat_id = self.__next_id("matchstat")

      # identification
      s1 = "%d,%d,%d," % (matchstat_id, match_id, person_id)
      
      # chequer play statistics
      chs = gs[ 'moves' ]
      m = chs[ 'marked' ]
      s2 = "%d,%d,%d,%d,%d,%d,%d,%f,%f,%f,%f,%d," % \
           ( chs[ 'total-moves' ], \
             chs[ 'unforced-moves' ], \
             m[ 'unmarked' ], \
             m[ 'good' ], \
             m[ 'doubtful' ], \
             m[ 'bad' ], \
             m[ 'very bad' ], \
             chs[ 'error-skill' ], \
             chs[ 'error-cost' ], \
             self.__calc_rate( chs[ 'error-skill' ], chs[ 'unforced-moves' ] ), \
             self.__calc_rate( chs[ 'error-cost' ], chs[ 'unforced-moves' ] ), \
             gnubg.errorrating( self.__calc_rate( chs[ 'error-skill' ],
                                             chs[ 'unforced-moves' ] ) ) ) 


      # luck statistics

      lus = gs[ 'dice' ]
      m = lus[ 'marked-rolls' ]
      s3 = "%d,%d,%d,%d,%d,%f,%f,%f,%f,%d," % \
           ( m[ 'verygood' ], \
             m[ 'good' ], \
             m[ 'unmarked' ], \
             m[ 'bad' ], \
             m[ 'verybad' ], \
             lus[ 'luck' ], \
             lus[ 'luck-cost' ], \
             self.__calc_rate( lus[ 'luck' ], chs[ 'total-moves' ] ), \
             self.__calc_rate( lus[ 'luck-cost' ], chs[ 'total-moves' ] ), \
             gnubg.luckrating( self.__calc_rate( lus[ 'luck' ],
                                            chs[ 'total-moves' ] ) ) ) 

      # cube statistics
   
      cus = gs[ 'cube' ]
      s4 = "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d," % \
           ( cus[ 'total-cube' ], \
             cus[ 'close-cube' ], \
             cus[ 'n-doubles' ], \
             cus[ 'n-takes' ], \
             cus[ 'n-drops' ], \
             cus[ 'missed-double-below-cp' ], \
             cus[ 'missed-double-below-cp' ], \
             cus[ 'wrong-double-below-dp' ], \
             cus[ 'wrong-double-above-tg' ], \
             cus[ 'wrong-take' ], \
             cus[ 'wrong-drop' ], \
             cus[ 'err-missed-double-below-cp-skill' ], \
             cus[ 'err-missed-double-above-cp-skill' ], \
             cus[ 'err-wrong-double-below-dp-skill' ], \
             cus[ 'err-wrong-double-above-tg-skill' ], \
             cus[ 'err-wrong-take-skill' ], \
             cus[ 'err-wrong-drop-skill' ], \
             cus[ 'err-missed-double-below-cp-cost' ], \
             cus[ 'err-missed-double-above-cp-cost' ], \
             cus[ 'err-wrong-double-below-dp-cost' ], \
             cus[ 'err-wrong-double-above-tg-cost' ], \
             cus[ 'err-wrong-take-cost' ], \
             cus[ 'err-wrong-drop-cost' ], \
             cus[ 'error-skill' ], \
             cus[ 'error-cost' ], \
             self.__calc_rate( cus[ 'error-skill' ], cus[ 'close-cube' ] ), \
             self.__calc_rate( cus[ 'error-cost' ], cus[ 'close-cube' ] ), \
             gnubg.errorrating( self.__calc_rate( cus[ 'error-skill' ],
                                             cus[ 'close-cube' ] ) ) ) 
      
      # overall
      
      s5 = "%f,%f,%f,%f,%d,%f,%f,%f," % \
           ( cus[ 'error-skill' ] + chs[ 'error-skill' ], \
             cus[ 'error-cost' ] + chs[ 'error-cost' ], \
             self.__calc_rate( cus[ 'error-skill' ] + chs[ 'error-skill' ], \
                          cus[ 'close-cube' ] + chs[ 'unforced-moves' ] ),
             self.__calc_rate( cus[ 'error-cost' ] + chs[ 'error-cost' ], \
                          cus[ 'close-cube' ] + chs[ 'unforced-moves' ] ),
             
             gnubg.errorrating( self.__calc_rate( cus[ 'error-skill' ] +
                                             chs[ 'error-skill' ], \
                                             cus[ 'close-cube' ] +
                                             chs[ 'unforced-moves' ] ) ),
             lus[ 'actual-result' ],
             lus[ 'luck-adjusted-result' ],
             self.__calc_rate( cus[ 'error-skill' ] + chs[ 'error-skill' ], \
                          chs[ 'total-moves' ] +
                          gs_other[ 'moves' ][ 'total-moves' ] ) )


      # for matches only
      if lus.has_key( 'fibs-rating-difference' ):
         s6 = '%f,' % ( lus[ 'fibs-rating-difference' ] )
      else:
         s6 = 'NULL,'

      if gs.has_key( 'error-based-fibs-rating' ):
         s = gs[ 'error-based-fibs-rating' ]
         if s.has_key( 'total' ):
            s6 = s6 + "%f," % ( s[ 'total' ] )
         else:
            s6 = s6 + "NULL,"
         if s.has_key( 'chequer' ):
            s6 = s6 + "%f," % ( s[ 'chequer' ] )
         else:
            s6 = s6 + "NULL,"
         if s.has_key( 'cube' ):
            s6 = s6 + "%f," % ( s[ 'cube' ] )
         else:
            s6 = s6 + "NULL,"
      else:
         s6 = s6 + "NULL,NULL,NULL,"

      # for money sessions only
      if gs.has_key( 'ppg-advantage' ):
         s = gs[ 'ppg-advantage' ]
         s7 = "%f,%f,%f,%f," % ( \
         s[ 'actual' ], \
         s[ 'actual-ci' ], \
         s[ 'luck-adjusted' ], \
         s[ 'luck-adjusted-ci' ] )
      else:
         s7 = "NULL,NULL,NULL,NULL,"

      # time penalties
      if gs.has_key( 'time' ):
         s8 = "%d,%f,%f" % \
           ( gs[ 'time' ][ 'time-penalty' ], \
             gs[ 'time' ][ 'time-penalty-skill' ], \
             gs[ 'time' ][ 'time-penalty-cost' ] )
      else:
         s8 = "0, 0, 0"
      
      query = "INSERT INTO matchstat VALUES (" + \
              s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + ");"
      cursor = self.conn.cursor()
      cursor.execute(query)
      cursor.close()
      
      return 0

   #
   # Add match
   # Parameters: conn (the database connection, DB API v2)
   #             env_id (the env)
   #             person_id0 (person_id of player 0)
   #             person_id1 (person_id of player 1)
   #             m (the gnubg match)             
   
   def __addMatch(self,env_id,person_id0,person_id1,m,key):

      # add match

      match_id = self.__next_id("match")
      
      mi = m[ 'match-info' ]
      
      if mi.has_key( 'date' ):
         date = "'%04d-%02d-%02d'" % ( mi[ 'date' ][ 2 ],
                                       mi[ 'date' ][ 1 ],
                                       mi[ 'date' ][ 0 ] )
      else:
         date = "NULL"

      # CURRENT_TIMESTAMP - SQL99, may have different names in various databases
      if DB_TYPE == DB_SQLITE:
         CURRENT_TIME = "datetime('now', 'localtime')"
      elif DB_TYPE == DB_POSTGRESQL:
         CURRENT_TIME = CURRENT_TIMESTAMP

      query = ("INSERT INTO match(match_id, checksum, env_id, person_id0, person_id1, " \
              "result, length, added, rating0, rating1, event, round, place, annotator, comment, date) " \
              "VALUES (%d, '%s', %d, %d, %d, %d, %d, " + CURRENT_TIME + ", '%s', '%s', '%s', '%s', '%s', '%s', '%s', %s) ") % \
              (match_id, key, env_id, person_id0, person_id1, \
               -1, mi[ 'match-length' ],
               self.__getKey( mi[ 'X' ], 'rating' )[0:80],
               self.__getKey( mi[ 'O' ], 'rating' )[0:80],
               self.__getKey( mi, 'event' )[0:80],
               self.__getKey( mi, 'round' )[0:80],
               self.__getKey( mi, 'place' )[0:80],
               self.__getKey( mi, 'annotator' )[0:80],
               self.__getKey( mi, 'comment' )[0:80], date )
      cursor = self.conn.cursor()
      cursor.execute(query)
      cursor.close()

      # add match statistics for both players

      if self.__addStat(match_id,person_id0,m[ 'stats' ][ 'X' ],
                   m [ 'stats' ][ 'O' ]) == None:
         print "Error adding player 0's stat to database."
         return None

      if self.__addStat(match_id,person_id1,m[ 'stats' ][ 'O' ],
                   m [ 'stats' ][ 'X' ]) == None:
         print "Error adding player 1's stat to database."
         return None
      
      return match_id
         
   
   #
   # Add match to database
   # Parameters: conn (the database connection, DB API v2)
   #             m (the match as obtained from gnubg)
   #             env_id (the environment of the match)
   # Returns: n/a
   #
   
   def __add_match(self,m,env_id,key):

      # add players

      mi = m[ 'match-info' ]
      
      player_id0 = self.__addPlayer( mi[ 'X' ][ 'name' ], env_id)
      if player_id0 == None:
         return None
      
      player_id1 = self.__addPlayer( mi[ 'O' ][ 'name' ], env_id)
      if player_id1 == None:
         return None
      
      # add match

      match_id = self.__addMatch( env_id, player_id0, player_id1, m, key)
      if match_id == None:
         return None

      return match_id

   # Simply function to run a query
   def __runquery(self, q):
   
      cursor = self.conn.cursor()
      cursor.execute(q);
      cursor.close()

   #
   # Check for existing match
   #
   #

   def is_existing(self, key):
   
      q = "SELECT match_id FROM match WHERE checksum = '%s'" % (key)

      c = self.conn.cursor()
      c.execute(q)
      row = c.fetchone()
      c.close()

      if row:
         return row[0]

      return -1

   #
   # Main logic
   #

   def createdatabase(self):

      cursor = self.conn.cursor()
      # Open file which has db create sql statments
      sqlfile = open("gnubg.sql", "r")
      done = False
      stmt = ""
      # Loop through file and run sql commands
      while not done:
        line = sqlfile.readline()
        if len(line) > 0:
           line = string.strip(line)
           if (line[0:2] != '--'):
              stmt = stmt + line
              if (len(line) > 0 and line[-1] == ';'):
                 cursor.execute(stmt)
                 stmt = ""
        else:
           done = True

      self.conn.commit()
      sqlfile.close()

   def connect(self):

      if DB_TYPE == DB_SQLITE:
         DBFILE = "data.db"
         import sqlite
      elif DB_TYPE == DB_POSTGRESQL:
         # Import the DB API v2 postgresql module
         import pgdb
         import _pg

      try:
         if DB_TYPE == DB_SQLITE:
            dbfile = os.access(DBFILE, os.F_OK)
            self.conn = sqlite.connect(DBFILE)
            if (dbfile == 0):
               self.createdatabase()

         elif DB_TYPE == DB_POSTGRESQL:
            self.conn = pgdb.connect(dsn=':gnubg')

         return self.conn
#if Postgresql
#      except _pg.error, e:
#         print e
      except:
         print "Unhandled exception..."
         traceback.print_exc()

      return None

   def disconnect(self):
         
      if self.conn == None:
         return

      self.conn.close()

   def addmatch(self,env_id=0,force=0):

      if self.conn == None:
         return -3

      m = gnubg.match(0,0,1,0)
      if m:
         key = gnubg.matchchecksum()
         existing_id = self.is_existing(key)
         if existing_id != -1:
            if (force):
               # Replace match in database - so first delete existing match
               self.__runquery("DELETE FROM matchstat WHERE match_id = %d" % existing_id);
               self.__runquery("DELETE FROM match WHERE match_id = %d" % existing_id);
            else:
               # Match is already in the database
               return -3
               
         match_id = self.__add_match(m,env_id,key)
         if match_id <> None:
            self.conn.commit()
            # Match successfully added to database
            return 0
         else:
            # Error add match to database
            return -1
      else:
         # No match
         return -2

   def test(self):

      if self.conn == None:
         return -1

      # verify that the main table is present
      rc = 0

      cursor = self.conn.cursor()
      try:
         cursor.execute("SELECT COUNT(*) from match;")
      except:
         rc = -2

      cursor.close()

      return rc

   def list_details(self, player):

      if self.conn == None:
         return None

      person_id = self.__playerExists(player, 0);
      
      if (person_id == -1):
        return -1

      stats = []

      cursor = self.conn.cursor()
      cursor.execute("SELECT count(*) FROM match WHERE person_id0 = %d OR person_id1 = %d" % \
         (person_id, person_id));
      row = cursor.fetchone()
      cursor.close()
      if row == None:
         played = 0
      else:
         played = int(row[0])
      stats.append(("Games played", played))

      cursor = self.conn.cursor()
      cursor.execute("SELECT * FROM match WHERE (person_id0 = %d and result = 1)" \
        " OR (person_id1 = %d and result = -1)" % (person_id, person_id));
      row = cursor.fetchone()
      cursor.close()
      if row == None:
         wins = 0
      else:
         wins = int(row[0])
      stats.append(("Games won", wins))

      cursor = self.conn.cursor()
      cursor.execute("SELECT AVG(overall_error_total) FROM matchstat" \
         " WHERE person_id = %d" % (person_id));
      row = cursor.fetchone()
      cursor.close()
      if row == None:
         rate = 0
      else:
         rate = row[0]
      stats.append(("Average error rate", rate))

      return stats

   def erase_player(self, player):

      if self.conn == None:
         return None

      person_id = self.__playerExists(player, 0);
      
      if (person_id == -1):
        return -1

      # from query to select all matches involving player
      mq = "FROM match WHERE person_id0 = %d OR person_id1 = %d" % \
         (person_id, person_id)

      # first remove any matchstats
      self.__runquery("DELETE FROM matchstat WHERE match_id in " \
         "(select match_id %s)" % (mq));

      # then remove any matches
      self.__runquery("DELETE " + mq);
      
      # then any nicks
      self.__runquery("DELETE FROM nick WHERE person_id = %d" % \
         (person_id));
      
      # finally remove the player record
      self.__runquery("DELETE FROM person WHERE person_id = %d" % \
         (person_id));

      self.conn.commit()
      
      return 1

   def erase_all(self):

      if self.conn == None:
         return None

      # first remove all matchstats
      self.__runquery("DELETE FROM matchstat");

      # then remove all matches
      self.__runquery("DELETE FROM match");
      
      # then all nicks
      self.__runquery("DELETE FROM nick");
      
      # finally remove the players
      self.__runquery("DELETE FROM person");

      self.conn.commit()
      
      return 1

   def select(self, str):

      if self.conn == None:
         return None

      cursor = self.conn.cursor()
      try:
         cursor.execute("SELECT " + str);
      except:
         cursor.close()
         return None

      all = cursor.fetchall()
      titles = [cursor.description[i][0] for i in range(len(cursor.description))]
      all.insert(0, titles)

      cursor.close()

      return all

   def update(self, str):

      if self.conn == None:
         return None

      cursor = self.conn.cursor()
      try:
         cursor.execute("UPDATE " + str);
      except:
         cursor.close()
         return None

      self.conn.commit()

      return 1
