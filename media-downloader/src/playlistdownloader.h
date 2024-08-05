/*
 *
 *  Copyright (c) 2021
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PLAYLIST_DOWNLOADER
#define PLAYLIST_DOWNLOADER

#include "utility.h"

#include "settings.h"
#include "context.hpp"
#include "downloadmanager.hpp"
#include "tableWidget.h"

class tabManager ;

class PlNetworkData
{
public:
	PlNetworkData() = default ;
	PlNetworkData( QByteArray data,QString errorString,int id,utility::MediaEntry e,tableWidget& t ) :
		m_networkData( std::move( data ) ),
		m_errorString( std::move( errorString ) ),
		m_id( id ),
		m_mediaEntry( std::make_shared< utility::MediaEntry >( std::move( e ) ) ),
		m_table( &t )
	{
	}
	const QByteArray& data() const
	{
		return m_networkData ;
	}
	const QString& errorString() const
	{
		return m_errorString ;
	}
	int id() const
	{
		return m_id ;
	}
	const utility::MediaEntry& media() const
	{
		return *m_mediaEntry ;
	}
	tableWidget& table() const
	{
		return *m_table ;
	}
	PlNetworkData move()
	{
		return std::move( *this ) ;
	}
private:
	QByteArray m_networkData ;
	QString m_errorString ;
	int m_id ;
	std::shared_ptr< utility::MediaEntry > m_mediaEntry ;
	tableWidget * m_table ;
};

Q_DECLARE_METATYPE( PlNetworkData )

class playlistdownloader : public QObject
{
        Q_OBJECT
public:
	playlistdownloader( Context& ) ;
	void init_done() ;
	void enableAll() ;
	void disableAll() ;
	void resetMenu() ;
	void retranslateUi() ;
	void tabEntered() ;
	void tabExited() ;
	void exiting() ;
	void saveData() ;
	void textAlignmentChanged( Qt::LayoutDirection ) ;
	void gotEvent( const QJsonObject& ) ;
	void updateEnginesList( const QStringList& ) ;
	void clipboardData( const QString& ) ;
private:
signals:
	void addTextToUiSignal( const QByteArray&,int ) ;
	void reportFinishedStatusSignal( const reportFinished&,const QStringList& ) ;
private:
	void reportFinishedStatus( const reportFinished&,const QStringList& ) ;
	void networkData( const utility::networkReply& ) ;
	void addTextToUi( const QByteArray&,int ) ;

	enum class size{ small,large,toggle } ;
	void resizeTable( playlistdownloader::size ) ;
	QString defaultEngineName() ;
	const engines::engine& defaultEngine() ;

	void customContextMenuRequested() ;
	void plSubscription() ;

	struct networkCtx
	{
		utility::MediaEntry media ;
		int id ;
		tableWidget& table ;
		networkCtx move()
		{
			return std::move( *this ) ;
		}
	} ;

	void networkResult( networkCtx,const utils::network::reply& ) ;

	void download() ;	
	void download( const engines::engine&,downloadManager::index ) ;
	void download( const engines::engine& ) ;
	void download( const engines::engine&,int ) ;

	void showBanner() ;
	void clearScreen() ;
	bool enabled() ;

	Context& m_ctx ;
	settings& m_settings ;
	Ui::MainWindow& m_ui ;
	QWidget& m_mainWindow ;
	tabManager& m_tabManager ;
	tableWidget m_table ;
	tableMiniWidget< int,2 > m_subscriptionTable ;
	bool m_gettingPlaylist = false ;
	bool m_showThumbnails ;
	bool m_autoDownload ;
	bool m_stoppedOnExisting ;
	bool m_dataReceived ;

	int m_networkRunning = 0 ;
	int m_downloaderId = -1 ;

	downloadManager m_ccmd ;

	utility::Terminator m_terminator ;

	QPixmap m_defaultVideoThumbnailIcon ;

	class customOptions
	{
	public:
		customOptions( QStringList&& opts,
			       const QString& downloadArchivePath,
			       const engines::engine& engine,
			       const Context& ctx ) ;
		const QStringList& options() const
		{
			return m_options ;
		}
		int maxMediaLength() const
		{
			return engines::engine::baseEngine::timer::toSeconds( m_maxMediaLength ) ;
		}
		int minMediaLength() const
		{
			return engines::engine::baseEngine::timer::toSeconds( m_minMediaLength ) ;
		}
		bool contains( const QString& e ) const ;
		bool breakOnExisting() const
		{
			return m_breakOnExisting ;
		}
		bool skipOnExisting() const
		{
			return m_skipOnExisting ;
		}
		customOptions move()
		{
			return std::move( *this ) ;
		}
	private:
		bool m_breakOnExisting = false ;
		bool m_skipOnExisting = false ;
		QStringList m_options ;
		QString m_maxMediaLength ;
		QString m_minMediaLength ;
		QByteArray m_archiveFileData ;
	};

	bool parseJson( const playlistdownloader::customOptions&,
			tableWidget& table,
			utility::MediaEntry ) ;

	void showEntry( tableWidget& table,tableWidget::entry e ) ;

	class subscription
	{
	public:
		subscription( const Context&,tableMiniWidget< int,2 >&,QWidget& ) ;
		void add( const QString& uiName,const QString& url,const QString& Opts ) ;
		void remove( int ) ;
		void setVisible( bool ) ;
		const QString& archivePath() const ;
		struct entry
		{
			entry()
			{
			}
			entry( const QString& u ) : url( u )
			{
			}
			entry( QString u,QString l,QString o ) :
				uiName( std::move( u ) ),
				url( std::move( l ) ),
				getListOptions( std::move( o ) )
			{
			}
			QString uiName ;
			QString url ;
			QString getListOptions ;
		};
		utility::vector< subscription::entry > entries() ;
	private:
		void save() ;
		QString m_path ;
		QString m_archivePath ;
		tableMiniWidget< int,2 >& m_table ;
		QWidget& m_ui ;
		QJsonArray m_array ;
	};

	class banner
	{
	public:
		banner( tableWidget& table) :
			m_table( table )
		{
		}
		void clear()
		{
			m_progress.clear() ;
			m_timer.reset() ;
		}
		void setBanner( const QString& s )
		{
			m_txt = s ;
		}
		void updateProgress( const QString& progress ) ;
		void reportError( const QString& ) ;
		void updateTimer() ;
		const QString& txt() const
		{
			return m_txt ;
		}
	private:
		tableWidget& m_table ;
		QString m_txt ;
		QString m_progress ;
		qint64 m_time ;
		engines::engine::baseEngine::timer m_timer ;
	} ;

	banner m_banner ;

	class stdError
	{
	public:
		stdError( playlistdownloader::banner& banner ) : m_banner( banner )
		{
		}
		bool operator()( const QByteArray& e ) ;
		stdError move()
		{
			return std::move( *this ) ;
		}
	private:
		playlistdownloader::banner& m_banner ;
	} ;

	class stdOut
	{
	public:
		stdOut( playlistdownloader& p,customOptions o,const engines::engine& e ) :
			m_parent( p ),m_engine( e ),m_customOptions( o.move() )
		{
		}
		void operator()( tableWidget& table,Logger::Data& data ) ;
		stdOut move()
		{
			return std::move( *this ) ;
		}
	private:
		playlistdownloader& m_parent ;
		const engines::engine& m_engine ;
		playlistdownloader::customOptions m_customOptions ;
	} ;

	class listIterator
	{
	public:
		listIterator( utility::vector< playlistdownloader::subscription::entry > s ) :
			m_list( s.move() )
		{
		}
		listIterator( const QString& s )
		{
			m_list.emplace_back( s ) ;
		}
		bool hasNext() const
		{
			return m_list.size() > 1 ;
		}
		const QString& url() const
		{
			return m_list.back().url ;
		}
		const QString& uiName() const
		{
			return m_list.back().uiName ;
		}
		const QString& listOptions() const
		{
			return m_list.back().getListOptions ;
		}
		playlistdownloader::listIterator next()
		{
			auto m = this->move() ;
			m.m_list.pop_back() ;
			return m ;
		}
		playlistdownloader::listIterator move()
		{
			return std::move( *this ) ;
		}
	private:
		utility::vector< playlistdownloader::subscription::entry > m_list ;
	} ;

	void getListing( playlistdownloader::listIterator,const engines::engine& ) ;
	void getList( playlistdownloader::listIterator,const engines::engine& ) ;
	void getList( customOptions&&,const engines::engine&,listIterator ) ;

	QByteArray m_jsonEndMarker = "0xdeadbeef>>MediaDownloaderEndMarker<<0xdeadbeef" ;
	subscription m_subscription ;
} ;

#endif
