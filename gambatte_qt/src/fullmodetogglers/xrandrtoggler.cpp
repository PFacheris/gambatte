/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "xrandrtoggler.h"

#include <QX11Info>

bool XRandRToggler::isUsable() {
	int unused;
	return XRRQueryExtension(QX11Info::display(), &unused, &unused) && XRRQueryVersion(QX11Info::display(), &unused, &unused);
}

XRandRToggler::XRandRToggler() :
originalResIndex(0),
fullResIndex(0),
rotation(0),
fullRateIndex(0),
originalRate(0),
isFull(false)
{
	XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), QX11Info::appRootWindow());
	fullResIndex = originalResIndex = XRRConfigCurrentConfiguration(config, &rotation);
	originalRate = XRRConfigCurrentRate(config);
	
	int nSizes;
	XRRScreenSize *randrSizes = XRRConfigSizes(config, &nSizes);
	
	for (int i = 0; i < nSizes; ++i) {
		ResInfo info;
		info.w = randrSizes[i].width;
		info.h = randrSizes[i].height;
		
		int nHz;
		short *rates = XRRConfigRates(config, i, &nHz);
		
		for (int j = 0; j < nHz; ++j)
			info.rates.push_back(rates[j]);
		
		infoVector.push_back(info);
	}
	
	{
		unsigned i = 0;
		
		while (i < infoVector[fullResIndex].rates.size() && infoVector[fullResIndex].rates[i] != originalRate)
			++i;
		
		if (i == infoVector[fullResIndex].rates.size())
			infoVector[fullResIndex].rates.push_back(originalRate);
		
		fullRateIndex = i;
	}
	
	XRRFreeScreenConfigInfo(config);
}

XRandRToggler::~XRandRToggler() {
	setFullMode(false);
// 	XRRFreeScreenConfigInfo(config);
}

const QRect XRandRToggler::fullScreenRect(const QWidget */*wdgt*/) const {
	int w, h;
	
	{
		XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), QX11Info::appRootWindow());
		
		int nSizes = 0;
		XRRScreenSize *randrSizes = XRRConfigSizes(config, &nSizes);
		
		Rotation rot = rotation;
		SizeID currentID = XRRConfigCurrentConfiguration(config, &rot);
		
		if (currentID < nSizes) {
			w = randrSizes[currentID].width;
			h = randrSizes[currentID].height;
		} else {
			w = infoVector[fullResIndex].w;
			h = infoVector[fullResIndex].h;
		}
		
		XRRFreeScreenConfigInfo(config);
	}
	
	return QRect(0, 0, w, h);
}

void XRandRToggler::setMode(unsigned /*screen*/, const unsigned resIndex, const unsigned rateIndex) {
	fullResIndex = resIndex;
	fullRateIndex = rateIndex;
	
	if (isFullMode())
		setFullMode(true);
}

void XRandRToggler::setFullMode(const bool enable) {
	XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), QX11Info::appRootWindow());
	
	SizeID currentID = XRRConfigCurrentConfiguration(config, &rotation);
	int currentRate = XRRConfigCurrentRate(config);
	
	SizeID newID;
	int newRate;
	
	if (enable) {
		newID = fullResIndex;
		newRate = infoVector[fullResIndex].rates[fullRateIndex];
		
		if (!isFull) {
			originalResIndex = currentID;
			originalRate = currentRate;
		}
	} else {
		newID = originalResIndex;
		newRate = originalRate;
	}
	
	if (newID != currentID || newRate != currentRate) {
		XRRSetScreenConfigAndRate(QX11Info::display(), config, QX11Info::appRootWindow(), newID, rotation, newRate, CurrentTime);
		
		if (newRate != currentRate)
			emit rateChange(newRate);
	}
	
	XRRFreeScreenConfigInfo(config);
	
	isFull = enable;
}

void XRandRToggler::emitRate() {
	XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), QX11Info::appRootWindow());
	emit rateChange(XRRConfigCurrentRate(config));
	XRRFreeScreenConfigInfo(config);
}