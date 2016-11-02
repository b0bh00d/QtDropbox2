#include "qdropbox2account.h"

QDropbox2User::QDropbox2User(QObject *parent) :
    QDropbox2Json(parent)
{
}

QDropbox2User::QDropbox2User(QString jsonString, QObject *parent) :
    QDropbox2Json(jsonString, parent)
{
    init();
}

QDropbox2User::QDropbox2User(const QDropbox2User& other) :
    QDropbox2Json()
{
    copyFrom(other);
}

void QDropbox2User::init()
{
    if(!isValid())
    {
        valid = false;
        return;
    }

    if(!hasKey("referral_link") ||
       !hasKey("name")  ||
       !hasKey("account_id") ||
       !hasKey("country") ||
       //!hasKey("quota_info") ||
       !hasKey("email"))
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "json invalid 1" << endl;
#endif
        valid = false;
        return;
    }

    _referralLink.setUrl(getString("referral_link"), QUrl::StrictMode);
    QDropbox2Json* name = getJson("name");
    _displayName    = name->getString("display_name");
    _id             = getString("account_id");
    _country        = getString("country");
    _email          = getString("email");
    _emailVerified  = getBool("email_verified");
    _locale         = getString("locale");
    _isPaired       = getBool("is_paired");
    QDropbox2Json* account_type = getJson("account_type");
    _type           = account_type->getString(".tag");
    _isDisabled     = getBool("disabled");
    if(hasKey("profile_photo_url"))
        _profilePhoto.setUrl(getString("profile_photo_url"), QUrl::StrictMode);

    valid = true;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "== account data ==" << endl;
    qDebug() << "reflink: " << _referralLink << endl;
    qDebug() << "displayname: " << _displayName << endl;
    qDebug() << "account_id: " << _id << endl;
    qDebug() << "country: " << _country << endl;
    qDebug() << "email: " << _email << endl;
    qDebug() << "== account data end ==" << endl;
#endif
}

QUrl QDropbox2User::referralLink() const
{
    return _referralLink;
}

QString QDropbox2User::displayName()  const
{
    return _displayName;
}

QString QDropbox2User::id()  const
{
    return _id;
}

QString QDropbox2User::country()  const
{
    return _country;
}

QString QDropbox2User::email()  const
{
    return _email;
}

bool QDropbox2User::emailVerified() const
{
    return _emailVerified;
}

QString QDropbox2User::locale() const
{
    return _locale;
}

bool QDropbox2User::isPaired() const
{
    return _isPaired;
}

QString QDropbox2User::type() const
{
    return _type;
}

bool QDropbox2User::isDisabled() const
{
    return _isDisabled;
}

QUrl QDropbox2User::profilePhoto() const
{
    return _profilePhoto;
}

QDropbox2User &QDropbox2User::operator =(QDropbox2User &a)
{
    copyFrom(a);
    return *this;
}

void QDropbox2User::copyFrom(const QDropbox2User &other)
{
    this->setParent(other.parent());
#ifdef QTDROPBOX_DEBUG
    qDebug() << "creating account from account" << endl;
#endif
    _referralLink  = other._referralLink;
    _id            = other._id;
    _displayName   = other._displayName;
    _email         = other._email;
    _emailVerified = other._emailVerified;
    _locale        = other._locale;
    _country       = other._country;
    _type          = other._type;
    _isDisabled    = other._isDisabled;
    _profilePhoto  = other._profilePhoto;
}

//------------------------------------------------------

QDropbox2Usage::QDropbox2Usage(QObject *parent) :
    QDropbox2Json(parent)
{
}

QDropbox2Usage::QDropbox2Usage(QString jsonString, QObject *parent) :
    QDropbox2Json(jsonString, parent)
{
    init();
}

QDropbox2Usage::QDropbox2Usage(const QDropbox2Usage& other) :
    QDropbox2Json()
{
    copyFrom(other);
}

void QDropbox2Usage::init()
{
    if(!isValid())
    {
        valid = false;
        return;
    }

    if(!hasKey("used") ||
       !hasKey("allocation"))
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "json invalid 1" << endl;
#endif
        valid = false;
        return;
    }

    _used           = getUInt64("used", true);
    QDropbox2Json* allocation = getJson("allocation");
    _allocated      = allocation->getUInt64("allocated", true);
    _allocationType = allocation->getString(".tag");

    valid = true;
}

quint64 QDropbox2Usage::used() const
{
    return _used;
}

QString QDropbox2Usage::allocationType() const
{
    return _allocationType;
}

quint64 QDropbox2Usage::allocated() const
{
    return _allocated;
}

QDropbox2Usage &QDropbox2Usage::operator =(QDropbox2Usage &a)
{
    copyFrom(a);
    return *this;
}

void QDropbox2Usage::copyFrom(const QDropbox2Usage &other)
{
    this->setParent(other.parent());
#ifdef QTDROPBOX_DEBUG
    qDebug() << "creating account from account" << endl;
#endif
    _used           = other._used;
    _allocationType = other._allocationType;
    _allocated      = other._allocated;
}
