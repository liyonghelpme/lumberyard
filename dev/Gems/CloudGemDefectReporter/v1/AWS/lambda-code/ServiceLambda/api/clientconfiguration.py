#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import service

import custom_field

@service.api
def get(request):
    return {'clientConfiguration': custom_field.get_client_configuration()}

@service.api
def put(request, client_configuration):
    return {'status': custom_field.update_client_configuration(client_configuration['clientConfiguration'])}