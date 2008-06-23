#ifndef SINGLE_SOURCE_COMPILE

/*
 * automatable_button.cpp - implementation of class automatableButton and
 *                          automatableButtonGroup
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "automatable_button.h"

#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>

#include "caption_menu.h"
#include "embed.h"
#include "main_window.h"




automatableButton::automatableButton( QWidget * _parent,
						const QString & _name ) :
	QPushButton( _parent ),
	boolModelView( new boolModel( FALSE, NULL, _name, TRUE ), this ),
	m_group( NULL )
{
	setAccessibleName( _name );
	doConnections();
}




automatableButton::~automatableButton()
{
	if( m_group != NULL )
	{
		m_group->removeButton( this );
	}
}




void automatableButton::modelChanged( void )
{
	if( QPushButton::isChecked() != model()->value() )
	{
		QPushButton::setChecked( model()->value() );
	}
}




void automatableButton::update( void )
{
	if( QPushButton::isChecked() != model()->value() )
	{
		QPushButton::setChecked( model()->value() );
	}
	QPushButton::update();
}




void automatableButton::contextMenuEvent( QContextMenuEvent * _me )
{
/*	if( model()->nullTrack() &&
			( m_group == NULL || m_group->model()->nullTrack() ) )*/
	if( m_group != NULL && !m_group->model()->isAutomated() )
	{
		QPushButton::contextMenuEvent( _me );
		return;
	}

	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	QString targetName;
	if ( m_group != NULL )
	{
		targetName = m_group->model()->displayName();
	}
	else
	{
		targetName = model()->displayName();
	}

	captionMenu contextMenu( targetName );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
}




void automatableButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
			engine::getMainWindow()->isCtrlPressed() == FALSE )
	{
		if( isCheckable() )
		{
			toggle();
		}
		_me->accept();
	}
	else
	{
		automatableModelView::mousePressEvent( _me );
		QPushButton::mousePressEvent( _me );
	}
}




void automatableButton::mouseReleaseEvent( QMouseEvent * _me )
{
	// TODO: Fix this. for example: LeftDown, RightDown, Both Released causes two events
	//   or - pressing down then releasing outside of the bbox causes click event.
	emit clicked();
}




void automatableButton::toggle( void )
{
	if( isCheckable() && m_group != NULL )
	{
		if( model()->value() == FALSE )
		{
			m_group->activateButton( this );
		}
	}
	else
	{
		model()->setValue( !model()->value() );
	}
}








automatableButtonGroup::automatableButtonGroup( QWidget * _parent,
						const QString & _name ) :
	QWidget( _parent ),
	intModelView( new intModel( 0, 0, 0, NULL, _name, TRUE ), this )
{
	hide();
	setAccessibleName( _name );
}




automatableButtonGroup::~automatableButtonGroup()
{
	for( QList<automatableButton *>::iterator it = m_buttons.begin();
					it != m_buttons.end(); ++it )
	{
		( *it )->m_group = NULL;
	}
}




void automatableButtonGroup::addButton( automatableButton * _btn )
{
	_btn->m_group = this;
	_btn->setCheckable( TRUE );
	_btn->model()->setValue( FALSE );
	// disable journalling as we're recording changes of states of 
	// button-group members on our own
	_btn->model()->setJournalling( FALSE );

	m_buttons.push_back( _btn );
	model()->setRange( 0, m_buttons.size() - 1 );
	updateButtons();
}




void automatableButtonGroup::removeButton( automatableButton * _btn )
{
	m_buttons.erase( qFind( m_buttons.begin(), m_buttons.end(), _btn ) );
	_btn->m_group = NULL;
}




void automatableButtonGroup::activateButton( automatableButton * _btn )
{
	if( _btn != m_buttons[model()->value()] &&
					m_buttons.indexOf( _btn ) != -1 )
	{
		model()->setValue( m_buttons.indexOf( _btn ) );
		foreach( automatableButton * btn, m_buttons )
		{
			btn->update();
		}
	}
}




void automatableButtonGroup::modelChanged( void )
{
	connect( model(), SIGNAL( dataChanged() ),
			this, SLOT( updateButtons() ) );
	intModelView::modelChanged();
	updateButtons();
}




void automatableButtonGroup::updateButtons( void )
{
	model()->setRange( 0, m_buttons.size() - 1 );
	int i = 0;
	foreach( automatableButton * btn, m_buttons )
	{
		btn->model()->setValue( i == model()->value() );
		++i;
	}
}


#include "automatable_button.moc"

#endif
