/*
 * Copyright 2011 Nate Koenig & Andrew Howard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#include <stdlib.h>

#include "sdf/sdf.h"
#include "sdf/sdf_parser.h"

#include "common/Console.hh"
#include "common/Color.hh"
#include "math/Vector3.hh"
#include "math/Pose.hh"

namespace sdf
{

////////////////////////////////////////////////////////////////////////////////
/// Init based on the installed sdf_format.xml file
bool init( SDFPtr _sdf )
{
  const char *path = getenv("GAZEBO_RESOURCE_PATH");
  if (path == NULL)
    gzerr << "GAZEBO_RESOURCE_PATH environment variable is not set\n";

  std::string filename = std::string(path) + "/sdf/gazebo.sdf";

  FILE *ftest = fopen(filename.c_str(), "r");
  if (ftest && initFile(filename, _sdf))
    return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool initFile(const std::string &_filename, SDFPtr _sdf)
{
  TiXmlDocument xmlDoc;
  if (xmlDoc.LoadFile(_filename))
  {
    return initDoc(&xmlDoc, _sdf);
  }
  else
    gzerr << "Unable to load file[" << _filename << "]\n";

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool initFile(const std::string &_filename, ElementPtr _sdf)
{
  TiXmlDocument xmlDoc;
  if (xmlDoc.LoadFile(_filename))
    return initDoc(&xmlDoc, _sdf);
  else
    gzerr << "Unable to load file[" << _filename << "]\n";

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool initString(const std::string &_xmlString, SDFPtr &_sdf)
{
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(_xmlString.c_str());

  return initDoc(&xmlDoc, _sdf);
}

////////////////////////////////////////////////////////////////////////////////
bool initDoc(TiXmlDocument *_xmlDoc, SDFPtr &_sdf)
{
  if (!_xmlDoc)
  {
    gzerr << "Could not parse the xml\n";
    return false;
  }

  TiXmlElement *xml = _xmlDoc->FirstChildElement("element");
  if (!xml)
  {
    gzerr << "Could not find the 'element' element in the xml file\n";
    return false;
  }

  return initXml(xml, _sdf->root);
}

////////////////////////////////////////////////////////////////////////////////
bool initDoc(TiXmlDocument *_xmlDoc, ElementPtr &_sdf)
{
  if (!_xmlDoc)
  {
    gzerr << "Could not parse the xml\n";
    return false;
  }

  TiXmlElement *xml = _xmlDoc->FirstChildElement("element");
  if (!xml)
  {
    gzerr << "Could not find the 'element' element in the xml file\n";
    return false;
  }

  return initXml(xml, _sdf);
}

////////////////////////////////////////////////////////////////////////////////
bool initXml(TiXmlElement *_xml, ElementPtr &_sdf)
{
  const char *nameString = _xml->Attribute("name");
  if (!nameString)
  {
    gzerr << "Element is missing the name attribute\n";
    return false;
  }
  _sdf->SetName( std::string(nameString) );

  const char *requiredString = _xml->Attribute("required");
  if (!requiredString)
  {
    gzerr << "Element is missing the required attributed\n";
    return false;
  }
  _sdf->SetRequired( requiredString );

  const char *elemTypeString = _xml->Attribute("type");
  if (elemTypeString)
  {
    bool required = std::string(requiredString) == "1" ? true : false;
    const char *elemDefaultValue = _xml->Attribute("default");
    _sdf->AddValue(elemTypeString, elemDefaultValue, required);
  }

  // Get all attributes
  for (TiXmlElement *child = _xml->FirstChildElement("attribute"); 
      child; child = child->NextSiblingElement("attribute"))
  {
    const char *name = child->Attribute("name");
    const char *type = child->Attribute("type");
    const char *defaultValue = child->Attribute("default");
    const char *requiredString = child->Attribute("required");
    if (!name)
    {
      gzerr << "Attribute is missing a name\n";
      return false;
    }
    if (!type)
    {
      gzerr << "Attribute is missing a type\n";
      return false;
    }
    if (!defaultValue)
    {
      gzerr << "Attribute[" << name << "] is missing a default\n";
      return false;
    }
    if (!requiredString)
    {
      gzerr << "Attribute is missing a required string\n";
      return false;
    }
    bool required = std::string(requiredString) == "1" ? true : false;

    _sdf->AddAttribute(name, type, defaultValue, required);
  }

  // Get all child elements
  for (TiXmlElement *child = _xml->FirstChildElement("element"); 
      child; child = child->NextSiblingElement("element"))
  {
    ElementPtr element(new Element);
    initXml(child, element);
    _sdf->elementDescriptions.push_back(element);
  }

  // Get all include elements
  for (TiXmlElement *child = _xml->FirstChildElement("include"); 
      child; child = child->NextSiblingElement("include"))
  {
    const char *path = getenv("GAZEBO_RESOURCE_PATH");
    if (path == NULL)
      gzerr << "GAZEBO_RESOURCE_PATH environment variable is not set\n";

    std::string filename = std::string(path) + "/sdf/" + child->Attribute("filename");

    ElementPtr element(new Element);

    initFile(filename, element);
    _sdf->elementDescriptions.push_back(element);
  }


  return true;
}


////////////////////////////////////////////////////////////////////////////////
bool readFile(const std::string &_filename, SDFPtr &_sdf)
{
  TiXmlDocument xmlDoc;
  xmlDoc.LoadFile(_filename);
  return readDoc(&xmlDoc, _sdf);
}

////////////////////////////////////////////////////////////////////////////////
bool readString(const std::string &_xmlString, SDFPtr &_sdf)
{
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(_xmlString.c_str());
  return readDoc(&xmlDoc, _sdf);
}

////////////////////////////////////////////////////////////////////////////////
bool readDoc(TiXmlDocument *_xmlDoc, SDFPtr &_sdf)
{
  if (!_xmlDoc)
  {
    gzerr << "Could not parse the xml\n";
    return false;
  }

  TiXmlElement* elemXml = _xmlDoc->FirstChildElement(_sdf->root->GetName());
   
  if (!readXml( elemXml, _sdf->root))
  {
    gzerr << "Unable to parse sdf element[" << _sdf->root->GetName() << "]\n";
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool readXml(TiXmlElement *_xml, ElementPtr &_sdf)
{
  if (!_xml)
  {
    if (_sdf->GetRequired() == "1" || _sdf->GetRequired() =="+")
    {
      gzerr << "SDF Element[" << _sdf->GetName() << "] is missing\n";
      return false;
    }
    else
      return true;
  }

  if (_xml->GetText() != NULL && _sdf->value)
  {
    _sdf->value->SetFromString( _xml->GetText() );
  }

  Param_V::iterator iter;
  TiXmlAttribute *attribute = _xml->FirstAttribute();
  // Iterate over all the attributes defined in the give XML element
  while (attribute)
  {
    // Find the matching attribute in SDF
    for (iter = _sdf->attributes.begin(); 
         iter != _sdf->attributes.end(); iter++)
    {
      if ( (*iter)->GetKey() == attribute->Name() )
      {
        // Set the value of the SDF attribute
        if ( !(*iter)->SetFromString( attribute->ValueStr() ) )
        {
          gzerr << "Unable to read attribute[" << (*iter)->GetKey() << "]\n";
          return false;
        }
        break;
      }
    }

    if (iter == _sdf->attributes.end())
      gzwarn << "XML Attribute[" << attribute->Name() << "] in element[" << _xml->Value() << "] not defined in SDF, ignoring.\n";

    attribute = attribute->Next();
  }

  // Check that all required attributes have been set
  for (iter = _sdf->attributes.begin(); 
       iter != _sdf->attributes.end(); iter++)
  {
    if ((*iter)->GetRequired() && !(*iter)->GetSet())
    {
      gzerr << "Required attribute[" << (*iter)->GetKey() << "] in element[" << _xml->Value() << "] is not specified in SDF.\n";
    }
  }

  // Read all the elements
  ElementPtr_V::iterator eiter;
  for (eiter = _sdf->elementDescriptions.begin(); 
       eiter != _sdf->elementDescriptions.end(); eiter++)
  {
    for (TiXmlElement* elemXml = _xml->FirstChildElement((*eiter)->GetName());
         elemXml; elemXml = elemXml->NextSiblingElement((*eiter)->GetName()))
    {
      ElementPtr element = (*eiter)->Clone();
      readXml( elemXml, element );
      _sdf->elements.push_back(element);
      if ((*eiter)->GetRequired() == "0" || (*eiter)->GetRequired() == "1")
        break;
    }
  }

  return true;
}

}
