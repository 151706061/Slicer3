/*=========================================================================

  Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   Module Description Parser
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Libs/ModuleDescriptionParser/ModuleDescription.h $
  Date:      $Date: 2010-01-22 14:43:05 -0500 (Fri, 22 Jan 2010) $
  Version:   $Revision: 11743 $

==========================================================================*/

#ifndef __ModuleDescriptionUtilities_h
#define __ModuleDescriptionUtilities_h

void replaceSubWithSub(std::string& s, const char *o, const char  *n);
void trimLeading(std::string& s, const char* extraneousChars = " \t\n"); 
void trimTrailing(std::string& s, const char* extraneousChars = " \t\n");
void trimLeadingAndTrailing(std::string& s, const char* extraneousChars = " \t\n");

#endif