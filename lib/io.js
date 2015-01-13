/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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

var IO = exports.IO =
{
  lineBreak: "\n",

  resolveFilePath: function(path)
  {
    return new FakeFile(_fileSystem.resolve(path));
  },

  readFromFile: function(file, listener, callback, timeLineID)
  {
    _fileSystem.read(file.path, function(result)
    {
      if (result.error)
        callback(result.error);
      else
      {
        var lines = result.content.split(/[\r\n]+/);
        for (var i = 0; i < lines.length; i++)
          listener.process(lines[i]);
        listener.process(null);
        callback(null);
      }
    });
  },

  writeToFile: function(file, data, callback, timeLineID)
  {
    var content = data.join(this.lineBreak) + this.lineBreak;
    _fileSystem.write(file.path, content, callback);
  },

  copyFile: function(fromFile, toFile, callback)
  {
    // Simply combine read and write operations
    var data = [];
    this.readFromFile(fromFile, {
      process: function(line)
      {
        if (line !== null)
          data.push(line);
      }
    }, function(e)
    {
      if (e)
        callback(e);
      else
        this.writeToFile(toFile, data, callback);
    }.bind(this));
  },

  renameFile: function(fromFile, newName, callback)
  {
    _fileSystem.move(fromFile.path, newName, callback);
  },

  removeFile: function(file, callback)
  {
    _fileSystem.remove(file.path, callback);
  },

  statFile: function(file, callback)
  {
    _fileSystem.stat(file.path, function(result)
    {
      callback(result.error, result);
    });
  }
};
