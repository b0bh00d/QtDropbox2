#pragma once

/*!
  Empty base class inherited by Dropbox entities (QDropbox2File, QDropbox2Folder,
  etc.) for homogeneous handling in containers.
 */
class QDROPBOXSHARED_EXPORT QDropbox2Entity
{
public:

    /*!
      Creates an empty instance of QDropbox2Entry.
      \param parent parent QObject
    */
    QDropbox2Entity() {}

    /*!
       Creates a copy of an other QDropbox2Entry instance.

       \param other original instance
     */
    QDropbox2Entity(const QDropbox2Entity & /*other*/) {}

    /*!
      Default destructor. Takes care of cleaning up when the object is destroyed.
    */
    virtual ~QDropbox2Entity() {}
};
