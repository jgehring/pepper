/**
 * libgithub - JavaScript library for visualizing GitHub data
 * Matt Sparks [http://quadpoint.org]
 *
 * Badge design from github-commit-badge by Johannes 'heipei' Gilger
 *
 * Customized for the pepper homepage at http://scm-pepper.sourceforge.net
 * by Jonas Gehring
 */
libgithub = new Object();


libgithub.Badge = function (username, repo, commitId)
{
  this._username = username;
  this._repo = repo;
  this._commitId = (typeof(commitId) == 'undefined') ? 'master' : commitId;
  this._gravatarSize = 60;
  this._startHidden = true;
  this._numCommits = 1;
  this._target = null;

  this.gravatarSize = function () { return this._gravatarSize; };
  this.gravatarSizeIs = function (size) { this._gravatarSize = size; };
  this.startHidden = function () { return this._startHidden; }
  this.startHiddenIs = function (setting) { this._startHidden = setting; };
  this.numCommits = function () { return this._numCommits; };
  this.numCommitsIs = function (numCommits) { this._numCommits = numCommits; };
  this.target = function () { return this._target; };
}


libgithub.Badge.prototype._userRepo = function (commit)
{
  var userRepo = _ce('div', {'class': 'libgithub-badge-username'});
  var link = _ce('a',
    {'href': 'http://github.com/' + this._username + '/' + this._repo,
     'target': '_blank',
     'class': 'libgithub-badge-username'});

  $(link).append(_tn(this._username + '/' + this._repo));
  $(userRepo).append(link);

  return userRepo;
}


libgithub.Badge.prototype._diffLine = function (commit)
{
  var diffLine = _ce('div', {'class': 'libgithub-badge-diffline'});

  $(diffLine).append(_tn('by '+ commit.committer.name));

  var d = new Date(commit.committed_date);
  var niceTime = d.strftime("%B %d, %Y, %H:%M %Z");
  var commitDate = _ce('span',
    {'class': 'libgithub-badge-text-date'});
  $(commitDate).append(_tn(niceTime));

  $(diffLine).append(document.createElement('br'));
  $(diffLine).append(_tn('at '));
  $(diffLine).append(commitDate);

  return diffLine;
}


libgithub.Badge.prototype._commitMessage = function (commit)
{
  var commitMessage = _ce('div',
    {'class': 'libgithub-badge-commitmessage'});

  var link = _ce('a',
    {'href': 'http://github.com' + commit.url,
     'target': '_blank',
     'class': 'libgithub-badge-commitid'});
  $(link).append(_tn( _truncate(commit.message, 80)));

  $(commitMessage).append(link);

  return commitMessage;
}


libgithub.Badge.prototype._diffStat = function (commit, fileList)
{
  var numAdded = ('added' in commit) ? commit.added.length : 0;
  var numRemoved = ('removed' in commit) ? commit.removed.length : 0;
  var numModified = ('modified' in commit) ? commit.modified.length : 0;

  var diffStat = _ce('div', {'class': 'libgithub-badge-diffstat'});
  var addedElement = _ce('span', {'class': 'libgithub-badge-diffadded'});
  var removedElement = _ce('span', {'class': 'libgithub-badge-diffremoved'});
  var modifiedElement = _ce('span', {'class': 'libgithub-badge-diffmodified'});

  $(diffStat).append(_tn('('));
  $(diffStat).append(_tn(numAdded));
  $(diffStat).append($(addedElement).text(' added'));
  $(diffStat).append(_tn(', '));
  $(diffStat).append(_tn(numRemoved));
  $(diffStat).append($(removedElement).text(' removed'));
  $(diffStat).append(_tn(', '));
  $(diffStat).append(_tn(numModified));
  $(diffStat).append($(modifiedElement).text(' modified'));
  $(diffStat).append(_tn(')'));

  if (numAdded + numRemoved + numModified > 0) {
    var username = this._username;
    var repo = this._repo;
    var showFilesLink = _ce('a',
      {'href': '#',
       'class': 'libgithub-badge-showMoreLink',
       'id': 'showMoreLink' + username + repo});

    $(showFilesLink).append(_tn('Show files'));
    $(diffStat).append(_tn(' '));
    $(diffStat).append(showFilesLink);

    $(showFilesLink).click(function () {
      $(fileList).slideToggle();

      if ($(this).text() == 'Show files')
        $(this).text('Hide files');
      else
        $(this).text('Show files');
      return false;
    });
  }

  return diffStat;
}


libgithub.Badge.prototype._fileList = function (commit)
{
  var filesAdded = ('added' in commit) ? commit.added : [];
  var filesRemoved = ('removed' in commit) ? commit.removed : [];
  var filesModified = ('modified' in commit) ? commit.modified : [];

  var fileList = _ce('div',
    {'class': 'libgithub-badge-filelist',
     'id': this._username + this._repo});
  var words = {'added': ['Added', filesAdded],
               'removed': ['Removed', filesRemoved],
               'modified': ['Modified', filesModified]};

  for (word in words) {
    var container = _ce('div');
    var ulist = _ce('ul');
    var cName = 'libgithub-badge-diff' + word;
    var text = words[word][0] + ':';
    var fset = words[word][1];
    var i = 0;

    $(container).append($(_ce('span', {'class': cName})).text(text));
    $(container).append(ulist);

    $.each(fset, function(j, f) {
      var file = _ce('li');
      if (word == 'modified')
        $(file).append(_tn(f.filename));
      else
        $(file).append(_tn(f));
      $(ulist).append(file);
      i++;
    });

    if (i > 0)
      $(fileList).append(container);
  }

  return fileList;
}


libgithub.Badge.prototype._badgeStructure = function (commit)
{
  var username = this._username;
  var repo = this._repo;
  var badgeDiv = _ce('div', {'class': 'libgithub-badge-outline'});
  var commitMessage = this._commitMessage(commit);
  var diffLine = this._diffLine(commit);

  $(badgeDiv).append(commitMessage);
  $(badgeDiv).append(diffLine);

  badgeDiv.hideFiles = function () {
    $(fileList).hide();
  };

  return badgeDiv;
}


libgithub.Badge.prototype._callback = function (data, element)
{
  if ('commit' in data) {
    var badgeDiv = this._badgeStructure(data.commit);
    element.append(badgeDiv);
    if (this._startHidden)
      badgeDiv.hideFiles();
  }
}


libgithub.Badge.prototype._showCommit = function (commitId, element)
{
  var _url = 'http://github.com/api/v2/json/commits/show/' +
              this._username + '/' + this._repo + '/' + commitId +
             '?callback=?';
  var _this = this;

  $.getJSON(_url, function (data) { _this._callback(data, element); });
}


libgithub.Badge.prototype._showCommits = function (branch, element)
{
  var _url = 'http://github.com/api/v2/json/commits/list/' +
             this._username + '/' + this._repo + '/' + branch +
             '?callback=?';
  var _this = this;

  $.getJSON(_url, function (data) {
    var maxLength = Math.min(_this._numCommits, data.commits.length);
    for (var i = 0; i < maxLength; ++i)
      _this._showCommit(data.commits[i].id, element);
  });
}


libgithub.Badge.prototype.targetIs = function (selector)
{
  this._target = selector;
  var _element = $(selector);

  if (this._numCommits > 1)
    this._showCommits(this._commitId, _element);
  else
    this._showCommit(this._commitId, _element);
}


/** utility functions **/

/**
 * truncate a string to a given length
 *
 * Args:
 *   string: string to truncate
 *   length: truncation length
 *   truncation: optional string to append to truncation (default: '...')
 *
 * Returns:
 *   truncated string
 */
var _truncate = function (string, length, truncation)
{
  length = length || 30;
  truncation = (typeof truncation == 'undefined') ? '...' : truncation;
  return string.length > length ?
    string.slice(0, length - truncation.length) + truncation : string;
};


/**
 * create an HTML element
 *
 * Args:
 *   tagName: name of tag
 *   attributes: map of attribute -> value pairs
 *
 * Returns:
 *   new element
 */
var _ce = function (tagName, attributes)
{
  var e = document.createElement(tagName)
  for (attr in attributes) {
    var value = attributes[attr];
    e.setAttribute(attr, value);
  }
  return e;
}


/**
 * create text node ready for insertion into HTML element
 *
 * Args:
 *   text: text node content
 *
 * Returns:
 *   new text node
 */
var _tn = function (text)
{
  return document.createTextNode(text);
}
