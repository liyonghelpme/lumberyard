/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "TestTypes.h"

#include <AzCore/XML/rapidxml.h>

#include <AzCore/std/string/string.h>

using namespace AZ;

namespace UnitTest
{
    /**
     * Rapid XML parser
     */
    class RapidXML
        : public AllocatorsFixture
    {
    public:
        RapidXML() {}

        void run()
        {
            AZStd::string xmlData("<?xml version=\"1.0\" encoding=\"utf-8\"?>\
                                    <rootnode version=\"1.0\" type=\"example\">\
                                        <childnode entry=\"1\">\
                                            <evendeepernode attr1=\"cat\" attr2=\"dog\"/>\
                                            <evendeepernode attr1=\"lion\" attr2=\"wolf\"/>\
                                        </childnode>\
                                        <childnode entry=\"2\">\
                                        </childnode>\
                                    </rootnode>");

            AZStd::string xmlBadData1("?");
            AZStd::string xmlBadData2("</autooptimizefile=0 /preset=ReferenceImage_Linear /reduce=-1 /colorspace=linear,linear");
            AZStd::string xmlBadData3("<?xml version=\"1.0\" encoding=\"UTF - 8\"?>\
                                       <note>\
                                            <to>Tove\
                                                <from>Jani\
                                                </to>\
                                             </from>\
                                            <heading>Reminder</heading>\
                                            <body>Don't forget me this weekend!</body>\
                                            </note>");

            rapidxml::xml_document<> doc;

            // Test error control with some bad XML input
            AZ_TEST_ASSERT(doc.parse<rapidxml::parse_full>(xmlBadData1.data()) == false);
            AZ_TEST_ASSERT(doc.isError());
            AZ_TEST_ASSERT(strlen(doc.getError()) > 0);

            AZ_TEST_ASSERT(doc.parse<rapidxml::parse_full>(xmlBadData2.data()) == false);
            AZ_TEST_ASSERT(doc.isError());
            AZ_TEST_ASSERT(strlen(doc.getError()) > 0);

            AZ_TEST_ASSERT(doc.parse<rapidxml::parse_full>(xmlBadData3.data()) == false);
            AZ_TEST_ASSERT(doc.isError());
            AZ_TEST_ASSERT(strlen(doc.getError()) > 0);

            // now test with correct XML
            AZ_TEST_ASSERT(doc.parse<rapidxml::parse_full>(xmlData.data()));
            AZ_TEST_ASSERT(doc.isError() == false);
            AZ_TEST_ASSERT(strlen(doc.getError()) == 0)

            // since we have parsed the XML declaration, it is the first node
            // (otherwise the first node would be our root node)
            const char* encoding = doc.first_node()->first_attribute("encoding")->value();
            AZ_TEST_ASSERT(strcmp("utf-8", encoding) == 0);
            // we didn't keep track of our previous traversal, so let's start again
            // we can match nodes by name, skipping the xml declaration entirely
            rapidxml::xml_node<>* cur_node = doc.first_node("rootnode");
            const char* rootnode_type = cur_node->first_attribute("type")->value();
            AZ_TEST_ASSERT(strcmp("example", rootnode_type) == 0);
            // go straight to the first evendeepernode
            cur_node = cur_node->first_node("childnode")->first_node("evendeepernode");
            const char* attr2 = cur_node->first_attribute("attr2")->value();
            AZ_TEST_ASSERT(strcmp("dog", attr2) == 0);
            // and then to the second evendeepernode
            cur_node = cur_node->next_sibling("evendeepernode");
            attr2 = cur_node->first_attribute("attr2")->value();
            AZ_TEST_ASSERT(strcmp("wolf", attr2) == 0);
        }
    };
}

AZ_TEST_SUITE(XML)
AZ_TEST(UnitTest::RapidXML)
AZ_TEST_SUITE_END

