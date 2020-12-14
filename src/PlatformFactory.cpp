/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <AdblockPlus/PlatformFactory.h>

#include "AsyncExecutor.h"
#include "DefaultFileSystem.h"
#include "DefaultLogSystem.h"
#include "DefaultPlatform.h"
#include "DefaultResourceReader.h"
#include "DefaultTimer.h"
#include "DefaultWebRequest.h"

using namespace AdblockPlus;

std::unique_ptr<Platform> PlatformFactory::CreatePlatform(CreationParameters&& parameters)
{
  if (!parameters.executor)
    parameters.executor = CreateExecutor();
  if (!parameters.logSystem)
    parameters.logSystem.reset(new DefaultLogSystem());
  if (!parameters.timer)
    parameters.timer.reset(new DefaultTimer());
  if (!parameters.webRequest)
  {
    parameters.webRequest.reset(
        new DefaultWebRequest(*parameters.executor, std::make_unique<DefaultWebRequestSync>()));
  }
  if (!parameters.fileSystem)
  {
    parameters.fileSystem.reset(new DefaultFileSystem(
        *parameters.executor,
        std::unique_ptr<DefaultFileSystemSync>(new DefaultFileSystemSync(parameters.basePath))));
  }
  if (!parameters.resourceReader)
    parameters.resourceReader.reset(new DefaultResourceReader());

  return std::make_unique<DefaultPlatform>(std::move(parameters));
}

std::unique_ptr<IExecutor> PlatformFactory::CreateExecutor()
{
  return std::unique_ptr<IExecutor>(new OptionalAsyncExecutor());
}
