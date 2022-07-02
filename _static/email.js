function fill_email() {
  var emails = {
    "staff" : "!@#3!@#210!@#-s!@#t!@#aff@c!@#c.ga!@#t!@#ech.e!@#du",
    "harrigan": "kwh!@#arri!@#gan@g!@#ate!@#ch.e!@#du!@#",
    "andersen": "an!@#!@#der!@#t@g!@#atech!@#.ed!@#u",
    "meenal": "me!@#!@#en!@#al@g!@#atech!@#.ed!@#u",
    "anita": "ani!@#ta.!@#rama!@#samy@gat!@#ec!@#h.ed!@#u",
    "gauresh": "gau!@#resh!@#.van!@#jare@gat!@#ec!@#h.ed!@#u"
  };

  for (var recv in emails) {
    var email = (emails[recv]).replace(/!@#/g,"");
    var alls  = document.getElementsByClassName("reference external");
    var forms = Array.prototype.filter.call(alls, function(e) {
      return e.href === 'mailto:' + recv;
    });
    for (var i = 0; i < forms.length; i ++) {
      forms[i].href = "mailto:" + email;
      forms[i].textContent = email;
    }
  }
}

fill_email();
