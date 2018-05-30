#include <iostream>
#include "tinyxml/tinyxml.h"
#include "tinyxml/tinystr.h"

int main() {
	TiXmlDocument doc;
	doc.LoadFile("data.xml");

	TiXmlElement* p = doc.FirstChildElement("Monsters");
	if (nullptr == p) {
		printf_s("Monsters not found!\n");
		system("pause");
		return 1;
	}
	TiXmlElement* orc = p->FirstChildElement("Orc");
	TiXmlElement* orcMsg = orc->FirstChildElement("Message");

	const char* valStr = orcMsg->Value();
	const char* textStr = orcMsg->GetText();

	printf("%s: %s\n", valStr, textStr);
	system("pause");
}