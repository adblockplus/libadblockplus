//
// No direct file system access, using FileSystem API
//

var IO = exports.IO =
{
  lineBreak: "\n",

  resolveFilePath: function(path)
  {
    return new FakeFile(path);
  },

  readFromFile: function(file, decode, listener, callback, timeLineID)
  {
    if ("spec" in file && /^defaults\b/.test(file.spec))
    {
      // Code attempts to read the default patterns.ini, we don't have that.
      // Make sure to execute first-run actions instead.
      callback(null);
      return;
    }

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

  writeToFile: function(file, encode, data, callback, timeLineID)
  {
    var content = data.join(this.lineBreak) + this.lineBreak;
    _fileSystem.write(file.path, content, callback);
  },

  copyFile: function(fromFile, toFile, callback)
  {
    // Simply combine read and write operations
    var data = [];
    this.readFromFile(fromFile, false, {
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
        this.writeToFile(toFile, false, data, callback);
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
