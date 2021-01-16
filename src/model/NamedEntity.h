/*
 * model/NamedEntity.h is part of Brewtarget, and is Copyright the following
 * authors 2020-2021
 * - Jeff Bailey <skydvr38@verizon.net>
 * - Matt Young <mfsy@yahoo.com>
 * - Mik Firestone <mikfire@gmail.com>
 * - Philip Greggory Lee <rocketman768@gmail.com>
 * - Samuel Östling <MrOstling@gmail.com>
 *
 * Brewtarget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Brewtarget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MODEL_NAMEDENTITY_H
#define _MODEL_NAMEDENTITY_H
#pragma once

#include <QDomText>
#include <QDomNode>
#include <QDomDocument>
#include <QList>
#include <QString>
#include <QObject>
#include <QMetaProperty>
#include <QVariant>
#include <QDateTime>
#include <QSqlRecord>
#include "brewtarget.h"
// For uintptr_t.
#if HAVE_STDINT_H
#   include <stdint.h>
#else
#   include "pstdint.h"
#endif

// Make uintptr_t available in QVariant.
Q_DECLARE_METATYPE( uintptr_t )

/*!
 * \class NamedEntity
 *
 * \brief The base class for our substantive storable items.
 *
 * Note that this class has previously been called \b Ingredient and \b BeerXMLElement, neither of which is an entirely
 * satisfactory name.  Some of the classes derived from this one (eg Instruction, Equipment, Style, Mash) are not really
 * ingredients in the normal sense of the word.  And the fact that derived classes can be instantiated from BeerXML is
 * not their defining characteristic (and indeed Instruction does not even represent something that can be stored in a
 * standard BeerXML document).
 */
class NamedEntity : public QObject
{
   Q_OBJECT
   Q_CLASSINFO("version","1")

   friend class Database;
   friend class BeerXML;
public:
   NamedEntity(Brewtarget::DBTable table, int key, QString t_name = QString(),
                  bool t_display = false, QString folder = QString());
   NamedEntity( NamedEntity const& other );

   // Our destructor needs to be virtual because we sometimes point to an instance of a derived class through a pointer
   // to this class -- ie NamedEntity * namedEntity = new Hop() and suchlike.  We do already get a virtual destructor by
   // virtue of inheriting from QObject, but this declaration does no harm.
   virtual ~NamedEntity() = default;

   // Everything that inherits from BeerXML has a name, delete, display and a folder
   Q_PROPERTY( QString name   READ name WRITE setName )
   Q_PROPERTY( bool deleted   READ deleted WRITE setDeleted )
   Q_PROPERTY( bool display   READ display WRITE setDisplay )
   Q_PROPERTY( QString folder READ folder WRITE setFolder )

   Q_PROPERTY( int key READ key )
   Q_PROPERTY( Brewtarget::DBTable table READ table )

   //! Convenience method to determine if we are deleted or displayed
   bool deleted() const;
   bool display() const;
   //! Access to the folder attribute.
   QString folder() const;
   //! Access to the name attribute.
   QString name() const;

   //! And ways to set those flags
   void setDeleted(const bool var, bool cachedOnly = false);
   void setDisplay(const bool var, bool cachedOnly = false);
   //! and a way to set the folder
   virtual void setFolder(const QString var, bool signal=true, bool cachedOnly = false);

   //!
   void setName(const QString var, bool cachedOnly = false);

   //! \returns our key in the table we are stored in.
   int key() const;
   //! \returns the table we are stored in.
   Brewtarget::DBTable table() const;
   //! \returns the BeerXML version of this element.
   int version() const;
   //! Convenience method to get a meta property by name.
   QMetaProperty metaProperty(const char* name) const;
   //! Convenience method to get a meta property by name.
   QMetaProperty metaProperty(QString const& name) const;

   // Some static helpers to convert to/from text.
   static double getDouble( const QDomText& textNode );
   static bool getBool( const QDomText& textNode );
   static int getInt( const QDomText& textNode );
   static QString getString( QDomText const& textNode );
   static QDateTime getDateTime( QDomText const& textNode );
   static QDate getDate( QDomText const& textNode );
   //! Convert the string to a QDateTime according to Qt::ISODate.
   static QDateTime getDateTime(QString const& str = "");
   static QDate getDate(QString const& str = "");
   static QString text(bool val);
   static QString text(double val);
   static QString text(int val);
   //! Convert the date to string in Qt::ISODate format for storage NOT display.
   static QString text(QDate const& val);

   //! Use this to pass pointers around in QVariants.
   static inline QVariant qVariantFromPtr( NamedEntity* ptr )
   {
      uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
      return QVariant::fromValue<uintptr_t>(addr);
   }

   static inline NamedEntity* extractPtr( QVariant ptrVal )
   {
      uintptr_t addr = ptrVal.value<uintptr_t>();
      return reinterpret_cast<NamedEntity*>(addr);
   }

   bool isValid();
   void invalidate();

   /*!
    * \brief Some entities (eg Fermentable, Hop) get copied when added to a recipe, but others (eg Instruction) don't.
    *        For those that do, we think of the copy as being a child of the original NamedEntity.  This function allows
    *        us to access that parent.
    * \return Pointer to the parent NamedEntity from which this one was originally copied, or null if no such parent exists.
    */
   virtual NamedEntity * getParent() = 0;

   void setParent(NamedEntity const & parentNamedEntity);

   /*!
    * \brief When we create an NamedEntity, or undelete a deleted one, we need to put it in the database.  For the case of
    *        undelete, it's helpful for the caller not to have to know what subclass of NamedEntity we are resurrecting.
    * \return Key of element inserted in database.
    */
   virtual int insertInDatabase() = 0;

signals:
   /*!
    * Passes the meta property that has changed about this object.
    * NOTE: when subclassing, be \em extra careful not to create a method with
    * the same signature. Otherwise, everything will silently break.
    */
   void changed(QMetaProperty, QVariant value = QVariant());
   void changedFolder(QString);
   void changedName(QString);

protected:

   //! The key of this entity in its table.
   int _key;
   //! The table where this entity is stored.
   Brewtarget::DBTable _table;
   // This is 0 if there is no parent (or parent is not yet known)
   int parentKey;

   /*!
    * \param prop_name A meta-property name
    * \param col_name The appropriate column in the table.
    * \param value the new value
    * \param notify true to call NOTIFY method associated with \c prop_name
    * Should do the following:
    * 1) Set the appropriate value in the appropriate table row.
    * 2) Call the NOTIFY method associated with \c prop_name if \c notify == true.
    */
   /*
   void set( const char* prop_name, const char* col_name, QVariant const& value, bool notify = true );
   void set( const QString& prop_name, const QString& col_name, const QVariant& value, bool notify = true );
   */
   void setEasy( QString prop_name, QVariant value, bool notify = true );

   /*!
    * \param col_name - The database column of the attribute we want to get.
    * Returns the value of the attribute specified by key/table/col_name.
    */
   QVariant get( const QString& col_name ) const;

   void setInventory( const QVariant& value, int invKey = 0, bool notify=true );
   QVariant getInventory( const QString& col_name ) const;

   QVariantMap getColumnValueMap() const;

private:
   /*!
    * \param valid - Indicates if the beerXML element was valid. There is a problem with importing invalid
    * XML. I'm hoping this helps fix it
    */
  bool _valid;
  mutable QString _folder;
  mutable QString _name;
  mutable QVariant _display;
  mutable QVariant _deleted;

};

typedef NamedEntity Ingredient; // .:TODO:. Temporary hack to keep Ingredient as a valid class name until we can refactor the rest of the code

#endif
