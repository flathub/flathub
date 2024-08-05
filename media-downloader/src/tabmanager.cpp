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

#include "tabmanager.h"
#include "proxy.h"

#include <QMimeData>
#include <QClipboard>
#include <QDateTime>

tabManager::tabManager( settings& s,
			translator& t,
			engines& e,
			Logger& l,
			Ui::MainWindow& ui,
			QWidget& w,
			MainWindow& mw,
			const QString& appName,
			utility::printOutPut& op ) :
	m_currentTab( s.tabNumber() ),
	m_ctx( s,t,ui,w,mw,l,e,*this,appName,op ),
	m_about( m_ctx ),
	m_configure( m_ctx ),
	m_basicdownloader( m_ctx ),
	m_batchdownloader( m_ctx ),
	m_playlistdownloader( m_ctx ),
	m_library( m_ctx )
{
	t.setContext( m_ctx ) ;

	qRegisterMetaType< QClipboard::Mode >() ;

	m_clipboard = QApplication::clipboard() ;

	if( m_clipboard ){

		auto m = Qt::QueuedConnection ;
		QObject::connect( m_clipboard,&QClipboard::changed,this,&tabManager::clipboardEvent,m ) ;
	}

	const auto& engines = m_ctx.Engines().getEngines() ;

	if( engines.size() > 0 ){

		ui.tabWidget->setCurrentIndex( 0 ) ;

		m_ctx.logger().updateView( true ) ;

		m_ctx.getVersionInfo().checkMediaDownloaderUpdate( engines ) ;
	}else{
		this->disableAll() ;

		ui.pbQuit->setEnabled( true ) ;
	}
}

void tabManager::init_done()
{
	this->setDefaultEngines() ;

	m_about.init_done() ;
	m_configure.init_done() ;
	m_basicdownloader.init_done() ;
	m_batchdownloader.init_done() ;
	m_playlistdownloader.init_done() ;
	m_library.init_done() ;

	utility::initDone() ;

	auto& m = m_ctx.Ui() ;
	auto& s = m_ctx.Settings() ;

	if( s.tabNumber() == 3 ){

		//We do not want to start with a library tab because it may hang the UI
		//before it is completely visible

		s.setTabNumber( 0 ) ;
	}

	m.tabWidget->setCurrentIndex( s.tabNumber() ) ;

	switch( s.tabNumber() ) {
		case 0 : m_basicdownloader.tabEntered() ; break ;
		case 1 : m_batchdownloader.tabEntered() ; break ;
		case 2 : m_playlistdownloader.tabEntered() ; break ;
		case 3 : m_library.tabEntered() ; break ;
		case 4 : m_configure.tabEntered() ; break ;
		case 5 : m_about.tabEntered() ; break ;
	}

	QObject::connect( m.tabWidget,&QTabWidget::currentChanged,[ this ]( int index ){

		switch( index ) {
			case 0 : m_basicdownloader.tabEntered() ; break ;
			case 1 : m_batchdownloader.tabEntered() ; break ;
			case 2 : m_playlistdownloader.tabEntered() ; break ;
			case 3 : m_library.tabEntered() ; break ;
			case 4 : m_configure.tabEntered() ; break ;
			case 5 : m_about.tabEntered() ; break ;
		}

		if( m_currentTab != index ){

			switch( m_currentTab ) {
				case 0 : m_basicdownloader.tabExited() ; break ;
				case 1 : m_batchdownloader.tabExited() ; break ;
				case 2 : m_playlistdownloader.tabExited() ; break ;
				case 3 : m_library.tabExited() ; break ;
				case 4 : m_configure.tabExited() ; break ;
				case 5 : m_about.tabExited() ; break ;
			}

			m_currentTab = index ;
		}
	} ) ;
}

void tabManager::setDefaultEngines()
{
	QStringList s ;

	for( const auto& engine : m_ctx.Engines().getEngines() ){

		if( engine.mainEngine() && engine.backendExists() && !engine.broken() ){

			s.append( engine.name() ) ;
		}
	}

	m_basicdownloader.updateEnginesList( s ) ;
	m_batchdownloader.updateEnginesList( s ) ;
	m_playlistdownloader.updateEnginesList( s ) ;
	m_configure.updateEnginesList( s ) ;
}

void tabManager::setProxy( const settings::proxySettings& proxy,const settings::proxySettings::type& m )
{
	proxy::set( m_ctx,m_firstTime,proxy.proxyAddress(),m ) ;
}

void tabManager::clipboardEvent( QClipboard::Mode mode )
{
	if( mode == QClipboard::Mode::Clipboard ){

		if( utility::platformIsWindows() ){

			this->bgThreadClipboardHandler() ;
		}else{
			this->mainThreadClipboardHandler() ;
		}
	}
}

void tabManager::mainThreadClipboardHandler()
{
	auto e = m_clipboard->mimeData() ;

	if( e && e->hasText() ){

		auto m = e->text() ;

		if( m.startsWith( "http" ) ){

			m_batchdownloader.clipboardData( m ) ;
		}
	}
}

class timeOutMonitor
{
public:
	timeOutMonitor( const Context& ctx ) :
		m_timeOut( ctx.Settings().timeOutWaitingForClipboardData() ),
		m_then( m_timeOut > 0 ? QDateTime::currentMSecsSinceEpoch() : 0 )
	{
	}
	bool notTimedOut() const
	{
		if( m_timeOut > 0 ){

			auto now = QDateTime::currentMSecsSinceEpoch() ;

			return ( now - m_then ) <= m_timeOut ;
		}else{
			return true ;
		}
	}
private:
	qint64 m_timeOut ;
	qint64 m_then ;
} ;

void tabManager::bgThreadClipboardHandler()
{
	utils::qthread::run( [ m = m_ctx.nativeHandleToMainWindow() ](){

		return utility::windowsGetClipBoardText( m ) ;

	},[ timer = timeOutMonitor( m_ctx ),this ]( const QString& e ){

		if( timer.notTimedOut() ){

			if( e.startsWith( "http" ) ){

				m_batchdownloader.clipboardData( e ) ;
			}
		}else{
			auto a = QObject::tr( "Warning: Skipping Clipboard Content" ) ;
			m_ctx.logger().add( a,utility::concurrentID() ) ;
		}
	} ) ;
}

tabManager& tabManager::gotEvent( const QByteArray& s )
{
	QJsonParseError err ;
	auto jsonDoc = QJsonDocument::fromJson( s,&err ) ;

	if( err.error == QJsonParseError::NoError ){

		auto e = jsonDoc.object() ;

		if( m_firstTime ){

			auto m = e.value( "--proxy" ).toString() ;

			if( m.isEmpty() ){

				auto s = m_ctx.Settings().getProxySettings() ;
				auto t = s.types() ;

				if( !t.none() ){

					this->setProxy( s,t ) ;
				}else{
					m_ctx.setNetworkProxy( m,m_firstTime ) ;
				}
			}else{
				m_ctx.setNetworkProxy( m,m_firstTime ) ;
			}

			m_firstTime = false ;
		}

		m_basicdownloader.gotEvent( e ) ;
		m_batchdownloader.gotEvent( e ) ;
		m_playlistdownloader.gotEvent( e ) ;
	}

	return *this ;
}

tabManager& tabManager::enableAll()
{
	m_about.enableAll() ;
	m_configure.enableAll() ;
	m_basicdownloader.enableAll() ;
	m_batchdownloader.enableAll() ;
	m_playlistdownloader.enableAll() ;
	m_library.enableAll() ;

	m_uiEnabled = true ;

	return *this ;
}

tabManager& tabManager::disableAll()
{
	m_about.disableAll() ;
	m_configure.disableAll() ;
	m_basicdownloader.disableAll() ;
	m_batchdownloader.disableAll() ;
	m_playlistdownloader.disableAll() ;
	m_library.disableAll() ;

	m_uiEnabled = false ;

	return *this ;
}

tabManager& tabManager::resetMenu()
{
	m_about.resetMenu() ;
	m_configure.resetMenu() ;
	m_basicdownloader.resetMenu() ;
	m_batchdownloader.resetMenu() ;
	m_playlistdownloader.resetMenu() ;
	m_library.resetMenu() ;

	return *this ;
}

tabManager& tabManager::reTranslateUi()
{
	m_about.retranslateUi() ;
	m_configure.retranslateUi() ;
	m_basicdownloader.retranslateUi() ;
	m_playlistdownloader.retranslateUi() ;
	m_library.retranslateUi() ;
	m_batchdownloader.retranslateUi() ;

	return *this ;
}

tabManager& tabManager::exiting()
{
	m_about.exiting() ;
	m_configure.exiting() ;
	m_basicdownloader.exiting() ;
	m_batchdownloader.exiting() ;
	m_playlistdownloader.exiting() ;
	m_library.exiting() ;

	return *this ;
}

void tabManager::textAlignmentChanged( Qt::LayoutDirection m )
{
	m_about.textAlignmentChanged( m ) ;
	m_configure.textAlignmentChanged( m ) ;
	m_basicdownloader.textAlignmentChanged( m ) ;
	m_batchdownloader.textAlignmentChanged( m ) ;
	m_playlistdownloader.textAlignmentChanged( m ) ;
	m_library.textAlignmentChanged( m ) ;
}
