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

#include "configure.h"
#include "tabmanager.h"
#include "mainwindow.h"
#include "networkAccess.h"
#include "utility.h"
#include "themes.h"
#include "mainwindow.h"
#include "versionInfo.h"

#include <QFileDialog>
#include <QFile>
#include <QDesktopServices>
#include <QClipboard>

configure::configure( const Context& ctx ) :
	m_ctx( ctx ),
	m_settings( m_ctx.Settings() ),
	m_ui( m_ctx.Ui() ),
	m_mainWindow( m_ctx.mainWidget() ),
	m_tabManager( m_ctx.TabManager() ),
	m_engines( m_ctx.Engines() ),
	m_tablePresetOptions( *m_ui.tableWidgetConfigurePresetOptions,m_ctx.mainWidget().font() ),
	m_tableUrlToDefaultEngine( *m_ui.tableWidgetConfigureUrl,m_ctx.mainWidget().font() ),
	m_tableDefaultDownloadOptions( *m_ui.tableWidgetEnginesDefaultOptions,m_ctx.mainWidget().font() ),
	m_presetOptions( m_ctx,m_settings ),
	m_downloadDefaultOptions( m_ctx,"downloadDefaultOptions.json" ),
	m_downloadEngineDefaultOptions( m_ctx,"downloadEngineDefaultOptions.json" )
{
	m_ui.pbConfigureCookiePath->setIcon( QIcon( ":/cookie" ) ) ;
	m_ui.pbOpenThemeFolder->setIcon( QIcon( ":/json" ) ) ;
	m_ui.pbOpenBinFolder->setIcon( QIcon( ":/executable" ) ) ;
	m_ui.pbConfigureDownloadPath->setIcon( QIcon( ":/folder" ) ) ;

	this->setVisibilityEditConfigFeature( false ) ;

	m_ui.tableWidgetConfigureUrl->setColumnWidth( 0,180 ) ;

	m_ui.tabWidgetConfigure->setCurrentIndex( 0 ) ;

	auto themesFolderPath = m_engines.engineDirPaths().themePath() ;

	themes ths( themesFolderPath ) ;

	auto themeName = m_settings.themeName() ;
	ths.setComboBox( *m_ui.comboBoxConfigureDarkTheme,themeName ) ;

	if( themeName.contains( "dark",Qt::CaseInsensitive ) ){

		if( utility::platformIsWindows() ){

			utility::windowsSetDarkModeTitleBar( m_ctx ) ;
		}
	}

	m_tablePresetOptions.setCurrentItemChanged( 0 ) ;

	m_tableUrlToDefaultEngine.setCurrentItemChanged( 0 ) ;

	m_tableDefaultDownloadOptions.setCurrentItemChanged( 0 ) ;

	connect( m_ui.pbConfigureSaveEditOption,&QPushButton::clicked,[ this ](){

		auto row = m_tableDefaultDownloadOptions.currentRow() ;

		if( row != -1 ){

			auto Old = m_tableDefaultDownloadOptions.item( row,1 ).text() ;

			auto New = m_ui.textEditConfigureEditOption->toPlainText() ;

			auto mm = m_ui.cbConfigureEngines->currentText() ;

			m_downloadEngineDefaultOptions.replace( mm,Old,New ) ;

			const auto& s = m_ctx.Engines().getEngineByName( mm ) ;

			this->populateOptionsTable( s.value(),row ) ;
		}

		this->setVisibilityEditConfigFeature( false ) ;
	} ) ;

	connect( m_ui.pbConfigureSaveEditOptionCancel,&QPushButton::clicked,[ this ](){

		this->setVisibilityEditConfigFeature( false ) ;
	} ) ;

	connect( m_ui.pbOpenThemeFolder,&QPushButton::clicked,[ themesFolderPath ](){

		QDesktopServices::openUrl( QUrl( "file:///" + themesFolderPath,QUrl::TolerantMode ) ) ;
	} ) ;

	connect( m_ui.pbOpenBinFolder,&QPushButton::clicked,[ this,themesFolderPath ](){

		const auto& m = m_engines.engineDirPaths().binPath() ;

		QDesktopServices::openUrl( QUrl( "file:///" + m,QUrl::TolerantMode ) ) ;
	} ) ;

	auto cc = static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ) ;

	connect( m_ui.comboBoxConfigureDarkTheme,cc,[ this,ths = ths.move() ]( int index ){

		if( index != -1 ){

			m_settings.setThemeName( ths.unTranslatedAt( index ) ) ;
		}
	} ) ;

	if( m_settings.showVersionInfoAndAutoDownloadUpdates() ){

		m_ui.comboBoxActionsWhenStarting->setCurrentIndex( 0 ) ;

	}else if( m_settings.showLocalAndLatestVersionInformation() ){

		m_ui.comboBoxActionsWhenStarting->setCurrentIndex( 1 ) ;

	}else if( m_settings.showLocalVersionInformationOnly() ){

		m_ui.comboBoxActionsWhenStarting->setCurrentIndex( 2 ) ;
	}else{
		m_ui.comboBoxActionsWhenStarting->setCurrentIndex( 3 ) ;
	}

	connect( m_ui.comboBoxActionsWhenStarting,cc,[ this ]( int index ){

		m_settings.setShowVersionInfoAndAutoDownloadUpdates( index == 0 ) ;
		m_settings.setShowLocalAndLatestVersionInformation( index == 1 ) ;
		m_settings.setShowLocalVersionInformationOnly( index == 2 ) ;
	} ) ;

	connect( m_ui.pbConfigureAddToPresetList,&QPushButton::clicked,[ this ](){

		auto a = m_ui.lineEditConfigureWebsite->text() ;
		auto b = m_ui.lineEditConfigureUiName->text() ;
		auto c = m_ui.lineEditConfigurePresetOptions->text() ;

		if( !a.isEmpty() && !b.isEmpty() ){

			m_tablePresetOptions.add( a,b,c ) ;

			m_tablePresetOptions.selectLast() ;

			m_ui.lineEditConfigureUiName->clear() ;
			m_ui.lineEditConfigurePresetOptions->clear() ;
			m_ui.lineEditConfigureWebsite->clear() ;
		}
	} ) ;

	m_tableUrlToDefaultEngine.connect( &QTableWidget::customContextMenuRequested,[ this ]( QPoint ){

		QMenu m ;

		connect( m.addAction( tr( "Remove" ) ),&QAction::triggered,[ this ](){

			auto m = m_tableUrlToDefaultEngine.currentRow() ;

			if( m != -1 ){

				m_downloadDefaultOptions.remove( m_tableUrlToDefaultEngine.stuffAt( m ) ) ;
				m_tableUrlToDefaultEngine.removeRow( m ) ;
				m_tableUrlToDefaultEngine.selectLast() ;
			}
		} ) ;

		m.exec( QCursor::pos() ) ;
	} ) ;

	m_tableDefaultDownloadOptions.connect( &QTableWidget::customContextMenuRequested,[ this ]( QPoint ){

		QMenu m ;

		connect( m.addAction( tr( "Set As Default" ) ),&QAction::triggered,[ this ](){

			auto m = m_tableDefaultDownloadOptions.currentRow() ;

			if( m != -1 ){

				auto mm = m_ui.cbConfigureEngines->currentText() ;

				const auto& s = m_ctx.Engines().getEngineByName( mm ) ;

				if( s ){
					auto obj = m_tableDefaultDownloadOptions.stuffAt( m ) ;

					m_downloadEngineDefaultOptions.setAsDefault( obj ) ;

					this->populateOptionsTable( s.value(),m ) ;
				}
			}
		} ) ;

		connect( m.addAction( tr( "Remove" ) ),&QAction::triggered,[ this ](){

			auto m = m_tableDefaultDownloadOptions.currentRow() ;

			if( m != -1 ){

				auto obj = m_tableDefaultDownloadOptions.stuffAt( m ) ;
				m_downloadEngineDefaultOptions.remove( obj ) ;

				auto mm = m_ui.cbConfigureEngines->currentText() ;

				const auto& s = m_ctx.Engines().getEngineByName( mm ) ;

				if( s ){

					this->populateOptionsTable( s.value() ) ;
				}
			}
		} ) ;

		connect( m.addAction( tr( "Copy" ) ),&QAction::triggered,[ this ](){

			auto s = m_tableDefaultDownloadOptions.currentRow() ;

			auto m = QApplication::clipboard() ;

			if( s != -1 && m ){

				auto obj = m_tableDefaultDownloadOptions.stuffAt( s ) ;

				auto str = obj.value( "options" ).toString() ;

				if( !str.isEmpty() ){

					m->setText( str ) ;
				}
			}
		} ) ;

		connect( m.addAction( tr( "Edit" ) ),&QAction::triggered,[ this ](){

			auto row = m_tableDefaultDownloadOptions.currentRow() ;

			if( row != -1 ){

				auto m = m_tableDefaultDownloadOptions.item( row,1 ).text() ;

				m_ui.textEditConfigureEditOption->setText( m ) ;

				this->setVisibilityEditConfigFeature( true ) ;
			}
		} ) ;

		m.exec( QCursor::pos() ) ;
	} ) ;

	connect( m_ui.pbAddDefaultDownloadOption,&QPushButton::clicked,[ this ](){

		auto m = m_ui.lineEditAddDefaultDownloadOption->text() ;

		if( !m.isEmpty() ){

			auto mm = m_ui.cbConfigureEngines->currentText() ;

			const auto& s = m_ctx.Engines().getEngineByName( mm ) ;

			if( s ){

				auto obj = m_downloadEngineDefaultOptions.addOpt( "yes",s->name(),m ) ;

				for( int i = 0 ; i < m_tableDefaultDownloadOptions.rowCount() ; i++ ){

					m_tableDefaultDownloadOptions.item( i,0 ).setText( "no" ) ;
				}

				m_tableDefaultDownloadOptions.add( std::move( obj ),"yes",m ) ;

				m_tableDefaultDownloadOptions.selectLast() ;

				m_ui.lineEditAddDefaultDownloadOption->clear() ;
			}
		}
	} ) ;

	connect( m_ui.pbConfigureManageUrl,&QPushButton::clicked,[ this ](){

		auto a = m_ui.lineEditConfigureManageUrl->text() ;
		auto b = m_ui.lineEditConfigureManageOptions->text() ;
		auto c = m_ui.cbConfigureEnginesUrlManager->currentText() ;

		if( !a.isEmpty() ){

			if( b.isEmpty() ){

				b = "Default" ;
			}

			m_ui.lineEditConfigureManageUrl->clear() ;
			m_ui.lineEditConfigureManageOptions->clear() ;

			auto obj = m_downloadDefaultOptions.add( a,b,c ) ;

			m_tableUrlToDefaultEngine.add( std::move( obj ),a,b ) ;

			m_tableUrlToDefaultEngine.selectLast() ;
		}
	} ) ;

	m_tablePresetOptions.connect( &QTableWidget::customContextMenuRequested,[ this ]( QPoint ){

		QMenu m ;

		connect( m.addAction( tr( "Remove" ) ),&QAction::triggered,[ this ](){

			auto row = m_tablePresetOptions.currentRow() ;

			if( row != -1 ){

				m_tablePresetOptions.removeRow( row ) ;
			}
		} ) ;

		connect( m.addAction( tr( "Copy" ) ),&QAction::triggered,[ this ](){

			auto row = m_tablePresetOptions.currentRow() ;
			auto col = m_tablePresetOptions.columnClicked() ;

			if( row != -1 && col != -1 ){

				auto mm = QApplication::clipboard() ;

				if( mm ){

					mm->setText( m_tablePresetOptions.item( row,col ).text() ) ;
				}
			}
		} ) ;

		m.exec( QCursor::pos() ) ;
	} ) ;

	connect( m_ui.cbConfigureEngines,cc,[ this ]( int index ){

		if( index != -1 ){

			auto m = m_ui.cbConfigureEngines->itemText( index ) ;

			auto s = m_engines.getEngineByName( m ) ;

			if( s ){

				this->populateOptionsTable( s.value() ) ;
			}

			this->setEngineOptions( m,engineOptions::options ) ;
		}
	} ) ;

	connect( m_ui.cbConfigureEnginesUrlManager,cc,[ this ]( int index ){

		if( index != -1 ){

			auto m = m_ui.cbConfigureEngines->itemText( index ) ;

			this->setEngineOptions( m,engineOptions::url ) ;
		}
	} ) ;

	connect( m_ui.pbConfigureCookiePath,&QPushButton::clicked,[ this ](){

		auto m = QFileDialog::getOpenFileName( &m_ctx.mainWidget(),tr( "Select A Cookie File" ),utility::homePath() ) ;

		if( !m.isEmpty() ){

			m_ui.lineEditConfigureCookiePath->setText( m ) ;
		}
	} ) ;

	connect( m_ui.pbConfigureQuit,&QPushButton::clicked,[ this ](){

		this->saveOptions() ;
		m_ctx.mainWindow().quitApp() ;
	} ) ;

	connect( m_ui.pbConfigureSave,&QPushButton::clicked,[ this ](){

		this->saveOptions() ;
	} ) ;

	connect( m_ui.cbShowTrayIcon,&QCheckBox::stateChanged,[ this ]( int s ){

		auto checked = s == Qt::CheckState::Checked ;

		m_settings.setshowTrayIcon( checked ) ;

		m_ctx.mainWindow().showTrayIcon( checked ) ;
	} ) ;

	connect( m_ui.pbConfigureAddAPlugin,&QPushButton::clicked,[ this ](){

		auto mm = tr( "Select An Engine File" ) ;
		auto ee = utility::homePath() ;

		auto m = QFileDialog::getOpenFileName( &m_ctx.mainWidget(),mm,ee ) ;

		if( m.isEmpty() ){

			return ;
		}

		auto id = utility::sequentialID() ;

		auto d = engines::file( m,m_ctx.logger() ).readAll() ;

		if( d.isEmpty() ){

			return ;
		}

		auto name = m_ctx.Engines().addEngine( d,util::split( m,'/',true ).last(),id ) ;

		if( name.isEmpty() ){

			return ;
		}

		auto& t = m_ctx.TabManager() ;

		t.basicDownloader().setAsActive() ;

		const auto& engine = m_ctx.Engines().getEngineByName( name ) ;

		if( engine ){

			class woof : public versionInfo::idone
			{
			public:
				woof( const Context& ctx,QString de ) :
					m_ctx( ctx ),m_defaultEngine( std::move( de ) )
				{
				}
				void operator()() override
				{
					if( m_success ){

						utility::setDefaultEngine( m_ctx,m_defaultEngine ) ;
					}
				}
				void failed() override
				{
					m_success = false ;
				}
			private:
				const Context& m_ctx ;
				QString m_defaultEngine ;
				bool m_success = true ;
			} ;

			auto& m = m_ctx.getVersionInfo() ;

			auto tt = util::types::type_identity< woof >() ;

			m.check( { engine.value(),id },{ tt,m_ctx,std::move( name ) },true ) ;
		}
	} ) ;

	connect( m_ui.pbConfigureRemoveAPlugin,&QPushButton::clicked,[ this ](){

		QMenu m ;

		for( const auto& it : m_ctx.Engines().enginesList() ){

			auto e = it ;
			m.addAction( e.replace( ".json","" ) )->setObjectName( it ) ;
		}

		m.addSeparator() ;

		m.addAction( tr( "Cancel" ) )->setEnabled( false ) ;

		connect( &m,&QMenu::triggered,[ & ]( QAction * ac ){

			auto id = utility::sequentialID() ;

			m_ctx.Engines().removeEngine( ac->objectName(),id ) ;

			auto& t = m_ctx.TabManager() ;

			t.basicDownloader().setAsActive() ;

			t.setDefaultEngines() ;
		} ) ;

		m.exec( QCursor::pos() ) ;
	} ) ;

	connect( m_ui.pbConfigureDownload,&QPushButton::clicked,[](){

	} ) ;

	connect( &m_menu,&QMenu::triggered,[ & ]( QAction * ac ){

		auto m = ac->objectName() ;

		if( !m.isEmpty() ){

			auto id = utility::sequentialID() ;

			if( m == m_ctx.appName() ){

				class meaw : public networkAccess::status
				{
				public:
					meaw( const Context& ctx,int id ) :
						m_ctx( ctx ),m_id( id )
					{
					}
					void done()
					{
						m_ctx.TabManager().enableAll() ;
					}
					int id()
					{
						return m_id ;
					}
				private:
					const Context& m_ctx ;
					int m_id ;
				} ;

				auto tt = util::types::type_identity< meaw >() ;

				m_ctx.network().updateMediaDownloader( { tt,m_ctx,id } ) ;
			}else{
				const auto& engine = m_ctx.Engines().defaultEngine( m,id ) ;

				this->downloadFromGitHub( { engine,id } ) ;
			}
		}
	} ) ;

	this->resetMenu() ;

	auto s = static_cast< void( QComboBox::* )( int ) >( &QComboBox::activated ) ;

	connect( m_ui.cbConfigureLanguage,s,[ & ]( int s ){

		const auto& languages = m_settings.localizationLanguages() ;

		if( s != -1 && s < languages.size() ){

			const auto& m = languages.at( s ) ;

			m_settings.setLocalizationLanguage( m ) ;

			m_ctx.Translator().setLanguage( m ) ;

			m_ctx.mainWindow().retranslateUi() ;

			m_tabManager.reTranslateUi() ;

			m_ctx.logger().reTranslateLogWindow() ;
		}
	} ) ;

	connect( m_ui.pbConfigureSetPresetDefaults,&QPushButton::clicked,[ this ](){

		m_presetOptions.setDefaults() ;
		m_tablePresetOptions.clear() ;

		this->showOptions() ;
	} ) ;

	connect( m_ui.pbConfigureDownloadPath,&QPushButton::clicked,[ this ](){

		auto a = tr( "Set Download Folder" ) ;
		auto b = QDir::homePath() ;
		auto c = QFileDialog::ShowDirsOnly ;

		auto e = QFileDialog::getExistingDirectory( &m_mainWindow,a,b,c ) ;

		if( !e.isEmpty() ){

			m_ui.lineEditConfigureDownloadPath->setText( e ) ;
		}
	} ) ;

	connect( m_ui.pbConfigureEngineDefaultOptions,&QPushButton::clicked,[ & ](){

		auto mm = m_ui.cbConfigureEngines->currentText() ;

		const auto& s = m_engines.getEngineByName( mm ) ;

		if( s ){

			m_downloadEngineDefaultOptions.removeAll( s->name() ) ;

			this->populateOptionsTable( s.value() ) ;
		}
	} ) ;

	m_ui.lineEditConfigureDownloadPath->setText( m_settings.downloadFolder() ) ;

	m_ui.cbAutoHideDownloadCompleted->setChecked( m_settings.autoHideDownloadWhenCompleted() ) ;

	auto ss = m_settings.showMetaDataInBatchDownloader() ;

	m_ui.cbConfigureShowMetaDataInBatchDownloader->setChecked( ss ) ;

	m_ui.cbAutoSaveNotDownloadedMedia->setChecked( m_settings.autoSavePlaylistOnExit() ) ;

	m_ui.lineEditConfigureTextEncoding->setText( m_settings.textEncoding() ) ;


	m_ui.cbShowTrayIcon->setChecked( m_settings.showTrayIcon() ) ;

	auto mm = QString::number( m_settings.maxConcurrentDownloads() ) ;

	m_ui.lineEditConfigureMaximuConcurrentDownloads->setText( mm ) ;

	const auto proxy      = m_ctx.Settings().getProxySettings() ;
	const auto proxy_type = proxy.types() ;

	if( !utility::platformIsWindows() ){

		auto systemProxyHeight = m_ui.rbUseSystemProxy->height() ;
		m_ui.rbUseSystemProxy->hide() ;

		const auto& fromEnv = m_ui.rbGetFromEnv ;

		fromEnv->move( fromEnv->x(),fromEnv->y() - systemProxyHeight ) ;

		const auto& manual = m_ui.rbUseManualProxy ;
		manual->move( manual->x(),manual->y() - systemProxyHeight ) ;
	}

	if( proxy_type.manual() ){

		m_ui.rbUseManualProxy->setChecked( true ) ;
		m_ui.lineEditCustormProxyAddress->setEnabled( true ) ;
		m_ui.labelProxy->setEnabled( true ) ;
	}else{
		m_ui.lineEditCustormProxyAddress->setEnabled( false ) ;
		m_ui.labelProxy->setEnabled( false ) ;

		if( proxy_type.env() ){

			m_ui.rbGetFromEnv->setChecked( true ) ;

		}else if( proxy_type.system() ){

			m_ui.rbUseSystemProxy->setChecked( true ) ;

		}else if( proxy_type.none() ){

			m_ui.rbNoProxy->setChecked( true ) ;
		}
	}

	m_ui.lineEditCustormProxyAddress->setText( proxy.proxyAddress() ) ;

	connect( m_ui.rbNoProxy,&QRadioButton::toggled,[ this ]( bool s ){

		if( s ){

			this->updateProxySettings( settings::proxySettings::Type::none ) ;
		}
	} ) ;

	connect( m_ui.rbUseManualProxy,&QRadioButton::toggled,[ this ]( bool s ){

		m_ui.lineEditCustormProxyAddress->setEnabled( s ) ;
		m_ui.labelProxy->setEnabled( s ) ;

		if( s ){

			auto a = settings::proxySettings::Type::manual ;
			auto e = m_ui.lineEditCustormProxyAddress->text() ;
			auto m = m_settings.getProxySettings() ;

			m_ctx.TabManager().setProxy( m.setProxySettings( a,e ),a ) ;
		}
	} ) ;

	connect( m_ui.rbUseSystemProxy,&QRadioButton::toggled,[ this ]( bool s ){

		if( s ){

			this->updateProxySettings( settings::proxySettings::Type::system ) ;
		}
	} ) ;

	connect( m_ui.rbGetFromEnv,&QRadioButton::toggled,[ this ]( bool s ){

		if( s ){

			this->updateProxySettings( settings::proxySettings::Type::env ) ;
		}
	} ) ;

	this->showOptions() ;
}

void configure::updateProxySettings( settings::proxySettings::Type s )
{
	m_ui.lineEditCustormProxyAddress->setEnabled( false ) ;
	m_ui.labelProxy->setEnabled( false ) ;
	m_ctx.TabManager().setProxy( m_settings.getProxySettings().setProxySettings( s ),s ) ;
}

void configure::init_done()
{
	m_tablePresetOptions.selectLast() ;
}

void configure::retranslateUi()
{
	this->resetMenu() ;

	this->setUpdateMenu() ;

	themes().setComboBox( *m_ui.comboBoxConfigureDarkTheme,m_settings.themeName() ) ;
}

void configure::setUpdateMenu()
{
	m_menu.clear() ;

	auto ac = m_menu.addAction( m_ctx.appName() ) ;

	ac->setObjectName( m_ctx.appName() ) ;

	ac->setEnabled( utility::platformIsWindows() ) ;

	m_menu.addSeparator() ;

	for( const auto& it : m_ctx.Engines().getEngines() ){

		if( it.validDownloadUrl() ){

			auto ac = m_menu.addAction( it.name() ) ;

			ac->setObjectName( it.name() ) ;

			ac->setEnabled( networkAccess::hasNetworkSupport() ) ;
		}
	}

	m_menu.addSeparator() ;

	m_menu.addAction( tr( "Cancel" ) ) ;

	m_ui.pbConfigureDownload->setMenu( &m_menu ) ;
}

void configure::downloadFromGitHub( const engines::Iterator& iter )
{
	m_ctx.network().download( iter ) ;
}

void configure::tabEntered()
{
	this->setUpdateMenu() ;

	auto mm = m_ui.cbConfigureEngines->currentText() ;

	auto s = m_engines.getEngineByName( mm ) ;

	if( s ){

		this->populateOptionsTable( s.value() ) ;
	}
}

void configure::populateOptionsTable( const engines::engine& s,int selectRow )
{
	m_tableDefaultDownloadOptions.clear() ;

	using mm = configure::downloadDefaultOptions::optsEngines ;

	m_downloadEngineDefaultOptions.forEach( [ &s,this ]( const mm& opts,QJsonObject obj ){

		if( s.name() == opts.engine ){

			m_tableDefaultDownloadOptions.add( std::move( obj ),opts.inuse,opts.options ) ;
		}

		return false ;
	} ) ;

	if( m_tableDefaultDownloadOptions.rowCount() == 0 ){

		const auto& e = s.defaultDownLoadCmdOptions() ;

		if( !e.isEmpty() ){

			auto b = e.join( " " ) ;

			auto obj = m_downloadEngineDefaultOptions.addOpt( "yes",s.name(),b ) ;

			m_tableDefaultDownloadOptions.add( std::move( obj ),"yes",b ) ;
		}
	}

	if( selectRow == -1 ){

		m_tableDefaultDownloadOptions.selectLast() ;
	}else{
		m_tableDefaultDownloadOptions.selectRow( selectRow ) ;
	}
}

void configure::tabExited()
{
	this->saveOptions() ;
}

void configure::updateEnginesList( const QStringList& e )
{
	auto& cb = *m_ui.cbConfigureEngines ;
	auto& xb = *m_ui.cbConfigureEnginesUrlManager ;

	cb.clear() ;
	xb.clear() ;

	for( const auto& it : e ){

		cb.addItem( it ) ;
		xb.addItem( it ) ;

		this->setEngineOptions( it,engineOptions::both ) ;
	}

	cb.setCurrentIndex( 0 ) ;
	xb.setCurrentIndex( 0 ) ;

	this->setEngineOptions( cb.currentText(),engineOptions::both ) ;
}

QString configure::engineDefaultDownloadOptions( const QString& engineName )
{
	QString options ;

	using mm = configure::downloadDefaultOptions::optsEngines ;

	m_downloadEngineDefaultOptions.forEach( [ & ]( const mm& opts,const QJsonObject& ){

		if( opts.engine == engineName && opts.inuse == "yes" ){

			options = opts.options ;

			return true ;
		}else{
			return false ;
		}
	} ) ;

	return options ;
}

QString configure::defaultDownloadOption()
{
	return "bestvideo[ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[ext=mp4]+bestaudio[ext=m4a]/bestvideo+bestaudio/best[ext=mp4]/best" ;
}

void configure::setDownloadOptions( int row,tableWidget& table )
{
	m_downloadDefaultOptions.setDownloadOptions( row,table ) ;
}

void configure::saveOptions()
{
	m_downloadDefaultOptions.save() ;
	m_downloadEngineDefaultOptions.save() ;

	auto m = m_ui.cbConfigureShowMetaDataInBatchDownloader->isChecked() ;

	m_ctx.TabManager().batchDownloader().setShowMetaData( m ) ;

	m_settings.setShowMetaDataInBatchDownloader( m ) ;
	m_settings.setDownloadFolder( m_ui.lineEditConfigureDownloadPath->text() ) ;
	m_settings.setAutoSavePlaylistOnExit( m_ui.cbAutoSaveNotDownloadedMedia->isChecked() ) ;
	m_settings.setTextEncoding( m_ui.lineEditConfigureTextEncoding->text() ) ;
	m_settings.setAutoHideDownloadWhenCompleted( m_ui.cbAutoHideDownloadCompleted->isChecked() ) ;

	auto s = m_ui.lineEditConfigureMaximuConcurrentDownloads->text() ;

	if( s.isEmpty() ){

		m_settings.setMaxConcurrentDownloads( 4 ) ;
	}else{
		bool ok ;

		auto m = s.toInt( &ok ) ;

		if( ok ){

			if( m == 0 ){

				m_settings.setMaxConcurrentDownloads( 4 ) ;
			}else{
				m_settings.setMaxConcurrentDownloads( m ) ;
			}
		}
	}

	auto mm = m_ui.cbConfigureEngines->currentText() ;

	const auto& ss = m_engines.getEngineByName( mm ) ;

	if( ss ){

		if( !ss->cookieArgument().isEmpty() ){

			auto m = m_ui.lineEditConfigureCookiePath->text() ;

			m_settings.setCookieFilePath( ss->name(),m ) ;
		}
	}

	settings::proxySettings::Type type = settings::proxySettings::Type::none ;

	if( m_ui.rbNoProxy->isChecked() ){

		type = settings::proxySettings::Type::none ;

	}else if( m_ui.rbUseSystemProxy->isChecked() ){

		type = settings::proxySettings::Type::system ;

	}else if( m_ui.rbGetFromEnv->isChecked() ){

		type = settings::proxySettings::Type::env ;

	}else if( m_ui.rbUseManualProxy->isChecked() ){

		type = settings::proxySettings::Type::manual ;
	}

	auto p = m_settings.getProxySettings() ;

	if( type == settings::proxySettings::Type::manual ){

		auto s = m_ui.lineEditCustormProxyAddress->text() ;

		m_ctx.TabManager().setProxy( p.setProxySettings( type,s ),type ) ;
	}else{
		m_ctx.TabManager().setProxy( p.setProxySettings( type ),type ) ;
	}

	this->savePresetOptions() ;
	m_ctx.TabManager().resetMenu() ;
}

void configure::setEngineOptions( const QString& e,engineOptions tab )
{
	const auto& s = m_engines.getEngineByName( e ) ;

	if( s ){

		auto _setUpUrl = [ & ](){

			const auto& engineName = s->name() ;

			m_tableUrlToDefaultEngine.clear() ;

			using mm = downloadDefaultOptions::opts ;

			auto& ss = m_downloadDefaultOptions ;

			ss.forEach( [ this,&engineName ]( const mm& e,QJsonObject obj ){

				if( engineName.isEmpty() || engineName == e.engine ){

					m_tableUrlToDefaultEngine.add( std::move( obj ),
								       e.url,
								       e.downloadOptions ) ;
				}

				return false ;
			} ) ;

			m_tableUrlToDefaultEngine.selectLast() ;
		} ;

		auto _setUpDownloadOptions = [ & ](){

			auto enable = !s->cookieArgument().isEmpty() ;

			auto mm = m_settings.cookieFilePath( s->name() ) ;

			m_ui.lineEditConfigureCookiePath->setText( mm ) ;
			m_ui.lineEditConfigureCookiePath->setEnabled( enable ) ;
			m_ui.pbConfigureCookiePath->setEnabled( enable ) ;
			m_ui.labelPathToCookieFile->setEnabled( enable ) ;
		} ;

		if( tab == engineOptions::url ){

			_setUpUrl() ;

		}else if( tab == engineOptions::options ){

			_setUpDownloadOptions() ;
		}else{
			_setUpUrl() ;
			_setUpDownloadOptions() ;
		}
	}
}

void configure::savePresetOptions()
{
	auto& table = m_tablePresetOptions.get() ;

	auto rowCount = table.rowCount() ;

	m_presetOptions.clear() ;

	for( int i = 0 ; i < rowCount ; i++ ){

		auto website = table.item( i,0 )->text() ;
		auto uiName  = table.item( i,1 )->text() ;
		auto options = table.item( i,2 )->text() ;

		const auto& e = m_tablePresetOptions.stuffAt( i ).value() ;

		if( e.isEmpty() ){

			m_presetOptions.add( uiName,options,website ) ;
		}else{
			m_presetOptions.add( e,options,website ) ;
		}
	}
}

void configure::showOptions()
{
	m_presetOptions.forEach( [ this ]( const configure::presetEntry& e ){

		m_tablePresetOptions.add( String{ e.uiName },e.website,e.uiNameTranslated,e.options ) ;
	} ) ;
}

void configure::resetMenu()
{
	const auto& languages = m_settings.localizationLanguages() ;

	const auto& language  = m_settings.localizationLanguage() ;

	auto& translator = m_ctx.Translator() ;

	m_ui.cbConfigureLanguage->clear() ;

	int index = 0 ;

	for( int i = 0 ; i < languages.size() ; i++ ){

		auto& e = languages[ i ] ;

		if( e == language ){

			index = i ;
		}

		m_ui.cbConfigureLanguage->addItem( translator.UIName( e ) ) ;
	}

	m_ui.cbConfigureLanguage->setCurrentIndex( index ) ;
}

void configure::exiting()
{
}

void configure::enableAll()
{
	const auto& s = m_engines.getEngineByName( m_ui.cbConfigureEngines->currentText() ) ;

	if( s ){

		auto enable = !s->cookieArgument().isEmpty() ;

		m_ui.lineEditConfigureCookiePath->setEnabled( enable ) ;
		m_ui.pbConfigureCookiePath->setEnabled( enable ) ;
		m_ui.labelPathToCookieFile->setEnabled( enable ) ;
	}

	m_ui.rbUseManualProxy->setEnabled( true ) ;
	m_ui.rbNoProxy->setEnabled( true ) ;
	m_ui.rbUseSystemProxy->setEnabled( utility::platformIsWindows() ) ;
	m_ui.rbGetFromEnv->setEnabled( true ) ;

	m_ui.lineEditCustormProxyAddress->setEnabled( m_ui.rbUseManualProxy->isChecked() ) ;
	m_ui.labelProxy->setEnabled( m_ui.rbUseManualProxy->isChecked() ) ;

	m_ui.label_6->setEnabled( true ) ;
	m_ui.label_3->setEnabled( true ) ;
	m_ui.label_4->setEnabled( true ) ;
	m_ui.label_5->setEnabled( true ) ;
	m_ui.lineEditConfigureTextEncoding->setEnabled( true ) ;
	m_ui.labelConfigureTextEncoding->setEnabled( true ) ;
	m_ui.pbOpenThemeFolder->setEnabled( true ) ;
	m_ui.pbOpenBinFolder->setEnabled( true ) ;
	m_ui.cbConfigureEnginesUrlManager->setEnabled( true ) ;
	m_ui.tableWidgetConfigureUrl->setEnabled( true ) ;
	m_ui.labelConfigureEngines_2->setEnabled( true ) ;
	m_ui.pbConfigureManageUrl->setEnabled( true ) ;
	m_ui.lineEditConfigureManageUrl->setEnabled( true ) ;
	m_ui.lineEditConfigureManageOptions->setEnabled( true ) ;
	m_ui.lineEditAddDefaultDownloadOption->setEnabled( true ) ;
	m_ui.tableWidgetEnginesDefaultOptions->setEnabled( true ) ;
	m_ui.pbAddDefaultDownloadOption->setEnabled( true ) ;
	m_ui.tableWidgetConfigurePresetOptions->setEnabled( true ) ;
	m_ui.lineEditConfigurePresetOptions->setEnabled( true ) ;
	m_ui.lineEditConfigureUiName->setEnabled( true ) ;
	m_ui.labelConfugureUiName->setEnabled( true ) ;
	m_ui.labelConfigureOptionsPresetOptiions->setEnabled( true ) ;
	m_ui.pbConfigureAddToPresetList->setEnabled( true ) ;
	m_ui.pbConfigureEngineDefaultOptions->setEnabled( true ) ;
	m_ui.cbConfigureEngines->setEnabled( true ) ;
	m_ui.labelConfigureEngines->setEnabled( true ) ;
	m_ui.pbConfigureSave->setEnabled( true ) ;
	m_ui.lineEditConfigureMaximuConcurrentDownloads->setEnabled( true ) ;
	m_ui.comboBoxConfigureDarkTheme->setEnabled( true ) ;
	m_ui.pbConfigureDownload->setEnabled( true ) ;
	m_ui.labelConfigureTheme->setEnabled( true ) ;
	m_ui.cbAutoSaveNotDownloadedMedia->setEnabled( true ) ;
	m_ui.cbConfigureLanguage->setEnabled( true ) ;
	m_ui.labelConfigureLanguage->setEnabled( true ) ;
	m_ui.lineEditConfigureDownloadPath->setEnabled( true ) ;
	m_ui.lineEditConfigureDownloadPath->setEnabled( true ) ;
	m_ui.pbConfigureDownloadPath->setEnabled( true ) ;
	m_ui.pbConfigureSetPresetDefaults->setEnabled( true ) ;
	m_ui.labelConfigureDownloadPath->setEnabled( true ) ;
	m_ui.pbConfigureQuit->setEnabled( true ) ;
	m_ui.pbConfigureAddAPlugin->setEnabled( true ) ;
	m_ui.pbConfigureRemoveAPlugin->setEnabled( true ) ;
	m_ui.cbConfigureShowMetaDataInBatchDownloader->setEnabled( true ) ;
	m_ui.labelMaximumConcurrentDownloads->setEnabled( true ) ;
	m_ui.cbShowTrayIcon->setEnabled( true ) ;
	m_ui.lineEditConfigureWebsite->setEnabled( true ) ;
	m_ui.labelConfugureWebSite->setEnabled( true ) ;
	m_ui.cbAutoHideDownloadCompleted->setEnabled( true ) ;
	m_ui.labelActionsAtStartup->setEnabled( true ) ;
	m_ui.comboBoxActionsWhenStarting->setEnabled( true ) ;
}

void configure::textAlignmentChanged( Qt::LayoutDirection z )
{
	auto a = m_ui.labelProxy ;
	auto b = m_ui.label_3 ;
	auto c = m_ui.label_4 ;
	auto d = m_ui.label_5 ;
	auto e = m_ui.label_6 ;
	auto f = m_ui.labelConfigureTextEncoding ;
	auto g = m_ui.labelConfigureEngines_2 ;
	auto h = m_ui.labelConfugureUiName ;
	auto i = m_ui.labelConfigureOptionsPresetOptiions ;
	auto j = m_ui.labelConfigureTheme ;
	auto k = m_ui.labelMaximumConcurrentDownloads ;
	auto l = m_ui.labelConfigureLanguage ;
	auto m = m_ui.labelConfigureDownloadPath ;
	auto n = m_ui.labelConfugureWebSite ;
	auto o = m_ui.labelActionsAtStartup ;

	utility::alignText( z,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o ) ;
}

void configure::disableAll()
{
	m_ui.rbUseManualProxy->setEnabled( false ) ;
	m_ui.rbNoProxy->setEnabled( false ) ;
	m_ui.rbUseSystemProxy->setEnabled( false ) ;
	m_ui.rbGetFromEnv->setEnabled( false ) ;
	m_ui.lineEditCustormProxyAddress->setEnabled( false ) ;
	m_ui.labelProxy->setEnabled( false ) ;
	m_ui.label_3->setEnabled( false ) ;
	m_ui.label_4->setEnabled( false ) ;
	m_ui.label_5->setEnabled( false ) ;
	m_ui.label_6->setEnabled( false ) ;
	m_ui.lineEditConfigureTextEncoding->setEnabled( false ) ;
	m_ui.labelConfigureTextEncoding->setEnabled( false ) ;
	m_ui.pbOpenThemeFolder->setEnabled( false ) ;
	m_ui.labelConfigureEngines_2->setEnabled( false ) ;
	m_ui.cbConfigureEnginesUrlManager->setEnabled( false ) ;
	m_ui.lineEditConfigureManageUrl->setEnabled( false ) ;
	m_ui.tableWidgetConfigureUrl->setEnabled( false ) ;
	m_ui.pbConfigureManageUrl->setEnabled( false ) ;
	m_ui.pbOpenBinFolder->setEnabled( false ) ;
	m_ui.lineEditAddDefaultDownloadOption->setEnabled( false ) ;
	m_ui.tableWidgetEnginesDefaultOptions->setEnabled( false ) ;
	m_ui.cbShowTrayIcon->setEnabled( false ) ;
	m_ui.pbAddDefaultDownloadOption->setEnabled( false ) ;
	m_ui.lineEditConfigureManageOptions->setEnabled( false ) ;
	m_ui.tableWidgetConfigurePresetOptions->setEnabled( false ) ;
	m_ui.lineEditConfigurePresetOptions->setEnabled( false ) ;
	m_ui.lineEditConfigureUiName->setEnabled( false ) ;
	m_ui.labelConfugureUiName->setEnabled( false ) ;
	m_ui.labelConfigureOptionsPresetOptiions->setEnabled( false ) ;
	m_ui.pbConfigureAddToPresetList->setEnabled( false ) ;
	m_ui.tableWidgetConfigurePresetOptions->setEnabled( false ) ;
	m_ui.labelPathToCookieFile->setEnabled( false ) ;
	m_ui.lineEditConfigureCookiePath->setEnabled( false ) ;
	m_ui.pbConfigureCookiePath->setEnabled( false ) ;
	m_ui.pbConfigureEngineDefaultOptions->setEnabled( false ) ;
	m_ui.cbAutoSaveNotDownloadedMedia->setEnabled( false ) ;
	m_ui.cbConfigureEngines->setEnabled( false ) ;
	m_ui.labelConfigureEngines->setEnabled( false ) ;
	m_ui.pbConfigureSave->setEnabled( false ) ;
	m_ui.comboBoxConfigureDarkTheme->setEnabled( false ) ;
	m_ui.pbConfigureDownload->setEnabled( false ) ;
	m_ui.labelConfigureTheme->setEnabled( false ) ;
	m_ui.lineEditConfigureMaximuConcurrentDownloads->setEnabled( false ) ;
	m_ui.labelMaximumConcurrentDownloads->setEnabled( false ) ;
	m_ui.pbConfigureAddAPlugin->setEnabled( false ) ;
	m_ui.pbConfigureRemoveAPlugin->setEnabled( false ) ;
	m_ui.pbConfigureDownload->setEnabled( false ) ;
	m_ui.cbConfigureLanguage->setEnabled( false ) ;
	m_ui.labelConfigureLanguage->setEnabled( false ) ;
	m_ui.pbConfigureQuit->setEnabled( false ) ;
	m_ui.lineEditConfigureDownloadPath->setEnabled( false ) ;
	m_ui.lineEditConfigureDownloadPath->setEnabled( false ) ;
	m_ui.pbConfigureDownloadPath->setEnabled( false ) ;
	m_ui.pbConfigureSetPresetDefaults->setEnabled( false ) ;
	m_ui.labelConfigureDownloadPath->setEnabled( false ) ;
	m_ui.cbConfigureShowMetaDataInBatchDownloader->setEnabled( false ) ;
	m_ui.lineEditConfigureWebsite->setEnabled( false ) ;
	m_ui.labelConfugureWebSite->setEnabled( false ) ;
	m_ui.cbAutoHideDownloadCompleted->setEnabled( false ) ;
	m_ui.labelActionsAtStartup->setEnabled( false ) ;
	m_ui.comboBoxActionsWhenStarting->setEnabled( false ) ;
}

configure::presetOptions::presetOptions( const Context& ctx,settings& s ) :
	m_path( ctx.Engines().engineDirPaths().dataPath( "presetOptions.json" ) )
{
	QJsonParseError err ;

	QSettings& m = s.bk() ;

	QByteArray data ;

	if( QFile::exists( m_path ) ){

		QFile f( m_path ) ;

		f.open( QIODevice::ReadOnly ) ;

		data = f.readAll() ;

	}else if( m.contains( "PresetJsonOptions" ) ){

		auto a = m.value( "PresetJsonOptions" ).toByteArray() ;

		m.remove( "PresetJsonOptions" ) ;

		data = QByteArray::fromHex( a ) ;
	}else{
		data = this->defaultData() ;
	}

	auto json = QJsonDocument::fromJson( data,&err ) ;

	if( err.error == QJsonParseError::NoError ){

		m_array = json.array() ;
	}
}

configure::presetOptions::~presetOptions()
{
	QFile f( m_path ) ;
	f.open( QIODevice::WriteOnly | QIODevice::Truncate ) ;
	f.write( QJsonDocument( m_array ).toJson( QJsonDocument::Indented ) ) ;
}

void configure::presetOptions::clear()
{
	auto s = m_array.size() ;

	for( int i = 0 ; i < s ; i++ ){

		m_array.removeAt( 0 ) ;
	}
}

void configure::presetOptions::setDefaults()
{
	this->clear() ;

	QJsonParseError err ;

	auto json = QJsonDocument::fromJson( this->defaultData(),&err ) ;

	if( err.error == QJsonParseError::NoError ){

		m_array = json.array() ;
	}
}

QString configure::presetOptions::optionsTranslated( const QString& e )
{
	for( const auto& s : util::asConst( m_array ) ) {

		auto obj = s.toObject() ;

		if( obj.value( "options" ).toString() == e ){

			auto mm = obj.value( "uiName" ).toString() ;

			return configure::presetEntry( mm,"","" ).uiNameTranslated ;
		}
	}

	return e ;
}

void configure::presetOptions::add( const QString& uiName,const QString& options,const QString& website )
{
	QJsonObject o ;

	o.insert( "uiName",uiName ) ;
	o.insert( "options",options ) ;
	o.insert( "website",website ) ;

	m_array.append( o ) ;
}

QString configure::optionsTranslated( const QString& e )
{
	return m_presetOptions.optionsTranslated( e ) ;
}

QByteArray configure::presetOptions::defaultData()
{
	return R"R([
    {
	"options": "-f bestvideo[height=144][ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[height=144]+bestaudio",
	"uiName": "Best Audio With Video Resolution Of 144p",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[height=240][ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[height=240]+bestaudio",
	"uiName": "Best Audio With Video Resolution Of 240p",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[height=360][ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[height=360]+bestaudio",
	"uiName": "Best Audio With Video Resolution Of 360p",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[height=480][ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[height=480]+bestaudio",
	"uiName": "Best Audio With Video Resolution Of 480p",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[height=720][ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[height=720]+bestaudio",
	"uiName": "Best Audio With Video Resolution Of 720p",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[height=1080][ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[height=1080]+bestaudio",
	"uiName": "Best Audio With Video Resolution Of 1080p",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[height=1440][ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[height=1440]+bestaudio",
	"uiName": "Best Audio With Video Resolution Of 1440p",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[height=2160][ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[height=2160]+bestaudio",
	"uiName": "Best Audio With Video Resolution Of 2160p",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[ext=mp4]+bestaudio[ext=m4a]/bestvideo+bestaudio/best[ext=mp4]/best",
	"uiName": "Best Available Audio Video",
	"website": "Youtube"
    },
    {
	"options": "-f bestaudio --extract-audio --audio-quality 0 --audio-format mp3 --embed-thumbnail",
	"uiName": "Best Available Audio Only(MP3)",
	"website": "Youtube"
    },
    {
	"options": "-f bestaudio",
	"uiName": "Best Available Audio Only",
	"website": "Youtube"
    },
    {
	"options": "-f bestaudio --extract-audio --audio-quality 0 --embed-thumbnail",
	"uiName": "Best Available Audio Only+Thumbnail",
	"website": "Youtube"
    },
    {
	"options": "-f bestvideo[ext=mp4][vcodec^=avc]+bestaudio[ext=m4a]/bestvideo[ext=mp4]+bestaudio[ext=m4a]/bestvideo+bestaudio/best[ext=mp4]/best",
	"uiName": "Best Available Audio Video",
	"website": "Other Websites"
    },
    {
	"options": "-f bestaudio/worst --embed-thumbnail --extract-audio --audio-quality 0 --audio-format mp3",
	"uiName": "Extract Audio As MP3",
	"website": "Other Websites"
    },
    {
	"options": "-f bestaudio/worst --extract-audio --audio-quality 0 --embed-thumbnail",
	"uiName": "Extract Audio",
	"website": "Other Websites"
    }
])R";
}

configure::presetEntry::presetEntry( const QString& ui,const QString& op,const QString& wb ) :
	uiName( ui ),options( op ),website( wb ),websiteTranslated( wb )
{
	if( uiName == "Extract Audio" ){

		uiNameTranslated = QObject::tr( "Extract Audio" ) ;

	}else if( uiName == "Extract Audio As MP3" ){

		uiNameTranslated = QObject::tr( "Extract Audio As MP3" ) ;

	}else if( uiName == "Best Available Audio Only" ){

		uiNameTranslated = QObject::tr( "Best Available Audio Only" ) ;

	}else if( uiName == "Best Available Audio Only(MP3)" ){

		uiNameTranslated = QObject::tr( "Best Available Audio Only(MP3)" ) ;

	}else if( uiName == "Best Available Audio Video" ){

		uiNameTranslated = QObject::tr( "Best Available Audio Video" ) ;

	}else if( uiName.startsWith( "Best Audio With Video Resolution Of" ) ){

		auto m = QObject::tr( "Best Audio With Video Resolution Of %1" ) ;

		uiNameTranslated = m.arg( uiName.mid( uiName.lastIndexOf( ' ' ) + 1 ) ) ;

	}else if( uiName == "Best Available Audio Only+Thumbnail" ){

		uiNameTranslated = QObject::tr( "Best Available Audio Only+Thumbnail" ) ;
	}else{
		uiNameTranslated = uiName ;
	}

	if( website == "Other Websites" ){

		websiteTranslated = QObject::tr( "Other Websites" ) ;

	}else if( website == "Youtube" ){

		websiteTranslated = QObject::tr( "Youtube" ) ;
	}
}

configure::downloadDefaultOptions::downloadDefaultOptions( const Context& ctx,const QString& name ) :
	m_path( ctx.Engines().engineDirPaths().dataPath( name ) )
{
	QJsonParseError err ;

	if( QFile::exists( m_path ) ){

		QFile f( m_path ) ;

		f.open( QIODevice::ReadOnly ) ;

		auto json = QJsonDocument::fromJson( f.readAll(),&err ) ;

		if( err.error == QJsonParseError::NoError ){

			m_array = json.array() ;
		}
	}
}

void configure::downloadDefaultOptions::save()
{
	QFile f( m_path ) ;
	f.open( QIODevice::WriteOnly | QIODevice::Truncate ) ;
	f.write( QJsonDocument( m_array ).toJson( QJsonDocument::Indented ) ) ;
}

void configure::setVisibilityEditConfigFeature( bool e )
{
	m_ui.pbConfigureSaveEditOption->setVisible( e ) ;
	m_ui.textEditConfigureEditOption->setVisible( e ) ;
	m_ui.labelEditConfigOptions->setVisible( e ) ;
	m_ui.pbConfigureSaveEditOptionCancel->setVisible( e ) ;
}

bool configure::downloadDefaultOptions::isEmpty( const QString& m )
{
	for( const auto& it : util::asConst( m_array ) ){

		auto s = it.toObject().value( "engineName" ).toString() ;

		if( s == m ){

			return false ;
		}
	}

	return true ;
}

void configure::downloadDefaultOptions::replace( const QString& engineName,
						 const QString& oldOptions,
						 const QString& newOptions )
{
	for( int i = 0 ; i < m_array.size() ; i++ ){

		auto obj = m_array[ i ].toObject() ;

		auto e = obj.value( "engineName" ).toString() ;
		auto o = obj.value( "options" ).toString() ;

		if( e == engineName && o == oldOptions ){

			obj.insert( "options",newOptions ) ;

			m_array[ i ] = std::move( obj ) ;
		}
	}
}

QJsonObject configure::downloadDefaultOptions::addOpt( const QString& inUse,
						       const QString& engineName,
						       const QString& options )
{
	if( inUse == "yes" ){

		for( int i = 0 ; i < m_array.size() ; i++ ){

			auto obj = m_array[ i ].toObject() ;

			obj.insert( "default","no" ) ;

			m_array[ i ] = std::move( obj ) ;
		}
	}

	QJsonObject obj ;

	obj.insert( "default",inUse ) ;
	obj.insert( "options",options ) ;
	obj.insert( "engineName",engineName ) ;

	m_array.append( obj ) ;

	return obj ;
}

QJsonObject configure::downloadDefaultOptions::add( const QString& url,
						    const QString& opts,
						    const QString& engineName )
{
	return this->add( { "contains",opts,engineName,url } ) ;
}

QJsonObject configure::downloadDefaultOptions::add( const configure::downloadDefaultOptions::opts& e )
{
	QJsonObject obj ;

	obj.insert( "comparator",e.comparator ) ;
	obj.insert( "downloadOption",e.downloadOptions ) ;
	obj.insert( "engine",e.engine ) ;
	obj.insert( "url",e.url ) ;

	m_array.append( obj ) ;

	return obj ;
}

void configure::downloadDefaultOptions::remove( const QJsonObject& e )
{
	for( int i = 0 ; i < m_array.size() ; i++ ){

		if( m_array[ i ].toObject() == e ){

			m_array.removeAt( i ) ;
			break ;
		}
	}

	auto engineName = e.value( "engineName" ) ;

	for( int i = 0 ; i < m_array.size() ; i++ ){

		auto obj = m_array[ i ].toObject() ;

		auto name = obj.value( "engineName" ) ;

		if( name == engineName ){

			auto s = obj.value( "default" ) ;

			if( s == "yes" ){

				return ;
			}
		}
	}

	for( int i = 0 ; i < m_array.size() ; i++ ){

		auto obj = m_array[ i ].toObject() ;

		auto name = obj.value( "engineName" ) ;

		if( name == engineName ){

			obj.insert( "default","yes" ) ;

			m_array[ i ] = std::move( obj ) ;

			break ;
		}
	}
}

void configure::downloadDefaultOptions::removeAll( const QString& e )
{
	while( [ this,&e ](){

		for( int i = 0 ; i < m_array.size() ; i++ ){

			auto s = m_array[ i ].toObject().value( "engineName" ) ;

			if( s == e ){

				m_array.removeAt( i ) ;
				return true ;
			}
		}

		return false ;
	}() ){} ;
}

void configure::downloadDefaultOptions::setAsDefault( const QJsonObject& e )
{
	for( int i = 0 ; i < m_array.size() ; i++ ){

		auto obj = m_array[ i ].toObject() ;
		obj.insert( "default","no" ) ;
		m_array[ i ] = std::move( obj ) ;
	}

	auto engineName = e.value( "engineName" ).toString() ;
	auto options = e.value( "options" ).toString() ;

	for( int i = 0 ; i < m_array.size() ; i++ ){

		auto obj = m_array[ i ].toObject() ;

		auto engine = obj.value( "engineName" ).toString() ;
		auto opts = obj.value( "options" ).toString() ;

		if( engine == engineName && options == opts ){

			obj.insert( "default","yes" ) ;

			m_array[ i ] = std::move( obj ) ;

			break ;
		}
	}
}
