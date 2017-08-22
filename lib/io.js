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

function readFileAsync(fileName)
{
  return new Promise((resolve, reject) =>
  {
    _fileSystem.read(fileName, (result) =>
    {
      if (result.error)
        return reject(result.error);
      resolve(result);
    });
  });
};

function writeFileAsync(fileName, content)
{
  return new Promise((resolve, reject) =>
  {
    _fileSystem.write(fileName, content, (error) =>
    {
      if (error)
        return reject(error);
      resolve();
    });
  });
};

exports.IO =
{
  lineBreak: "\n",

  readFromFile(fileName, listener)
  {
    return readFileAsync(fileName).then((result) =>
    {
      let lines = result.content.split(/[\r\n]+/);
      for (let line of lines)
        listener(line);
    });
  },

  writeToFile(fileName, generator)
  {
    let content = Array.from(generator).join(this.lineBreak) + this.lineBreak;
    return writeFileAsync(fileName, content);
  },

  copyFile(fromFileName, toFileName)
  {
    return readFileAsync(fromFileName).then(content => writeFileAsync(toFileName, content));
  },

  renameFile(fromFileName, newNameFile)
  {
    return new Promise((resolve, reject) =>
    {
      _fileSystem.move(fromFileName, newNameFile, (error) =>
      {
        if (error)
          return reject(error);
        resolve();
      });
    });
  },

  removeFile(fileName)
  {
    return new Promise((resolve, reject) =>
    {
      _fileSystem.remove(fileName, (error) =>
      {
        if (error)
          return reject(error);
        resolve();
      });
    });
  },

  statFile(fileName, callback)
  {
    return new Promise((resolve, reject) =>
    {
      _fileSystem.stat(fileName, (result) =>
      {
        if (result.error)
          return reject(result.error);
        resolve(result);
      });
    });
  }
};
