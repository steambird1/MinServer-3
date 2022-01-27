
// Note: set it to your own assiocation.
const service_head = "/services";

// Usage of exceptions
function msexception(msg, errid) {
    this.error = errid;
    this.message = msg;
}

const file_override = "w";
const file_append = "a";
const file_binary = "b";
// e.g. readfile(filename)
//      readfile(filename, file_binary)
//      writefile(filename, file_override + file_binary, content)

// Usage: var x = new msuser();
function msuser(token, xhobject, f_operate, u_operate, d_operate, myuid) {

    this.xhobject = xhobject;
    this.token = token;
    this.f_operate = f_operate;
    this.u_operate = u_operate;
    this.d_operate = d_operate;
    this.myuid = myuid;

    // Operations of content

    this.readfile = function (filename, mode) {
        var wmode = mode;
        if (mode == null || mode == undefined) {
            wmode = "";
        }
        this.xhobject.open("GET", this.f_operate + "filename=" + filename + "&mode=r" + wmode + "&token=" + this.token);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        } else {
            return this.xhobject.responseText;
        }
    }

    this.writefile = function (filename, mode, content) {
        this.xhobject.open("POST", this.f_operate + "filename=" + filename + "&mode=" + mode + "&token=" + this.token);
        this.xhobject.send(content);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    // Operations of ownership

    this.chown = function (filename, chto) {
        this.xhobject.open("GET", this.u_operate + "operate=chown&file=" + filename + "&token=" + this.token + "&touid=" + chto, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    this.chperm = function (filename, chto, perm) {
        this.xhobject.open("GET", this.u_operate + "operate=chperm&file=" + filename + "&token=" + this.token + "&touid=" + chto + "&toperm=" + perm, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    this.modify = function (new_pass) {
        this.xhobject.send("GET", this.u_operate + "operate=create&id=" + this.myuid + "&passwd=" + new_pass + "&token=" + this.token, false);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    this.upload = function (content, target, jump_to) {
        wjumpto = "&jumpto=" + jump_to;
        if (jump_to == undefined) {
            wjumpto = "";
        }
        this.xhobject.send("POST", this.d_operate + "utoken=" + this.token + "&name=" + target + wjumpto, false);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    this.logout = function () {
        this.xhobject.open("GET", this.u_operate + "operate=logout&token=" + this.token, false);
        this.xhobject.send(null);
        this.token = null;
    }

}

// Usage: var x = new mslib();

function mslib() {
    this.xhobject = null;

    if (window.XMLHttpRequest) {
        this.xhobject = new XMLHttpRequest();
    } else if (window.ActiveXObject) {
        this.xhobject = new ActiveXObject("Microsoft.XMLHTTP");
    } else {
        throw new msexception("This browser does not support XMLHTTP object for connection!", -1);
    }

    // Operations
    this.f_operate = service_head + "?method=file_operate&";
    this.u_operate = service_head + "?method=auth_workspace&";
    this.d_operate = service_head + "?method=upload&"
    this.default_user = new msuser(0, this.xhobject, this.f_operate, this.u_operate, this.d_operate, 0);

    this.auth = function (uid, passwd) {
        this.xhobject.open("GET", this.u_operate + "operate=check&request=" + uid + "&passwd=" + passwd, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Incorrect user name or password", 2);
        } else {
            return new msuser(this.xhobject.responseText, this.xhobject, this.f_operate, this.u_operate, this.d_operate, uid);
        }
    }

    this.request = function (operator, path, content) {
        this.xhobject.open(operator, path, false);
        this.xhobject.send(null);
        return this.xhobject.responseText;
    }

    this.status = function () {
        return this.xhobject.status;
    }

    this.call = function (dll, parameter, method, content) {
        throw new msexception("This function is not in support now", 4)
    }

    this.register = function (passwd, uid) {
        ider = "";
        if (uid == undefined) {
            ider = "";
        } else {
            ider = "&id=" + uid;
        }
        this.xhobject.send("GET", this.u_operate + "operate=create" + ider + "&passwd=" + passwd, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("User exists and requires login", 2);
        }
        if (ider == null) {
            return this.xhobject.responseText;
        }
    }

}

// Some tools useful
// Input: MinServer key-value object (like pageinfo.head_args)
function toJSDictionary(mins_dict_obj) {
    var res = new Object();
    for (var i = 0; i < mins_dict_obj.length; i++) {
        res[mins_dict_obj[i].key] = mins_dict_obj[i].value;
    }
    return res;
}

function translateHTMLChar(hchar) {
    var res = "";
    var i = 0;
    for (; i < hchar.length; i++) {
        if (hchar[i] == "%") {
            res += String.fromCharCode(parseInt(hchar[i + 1] + hchar[i + 2], 16));
            i += 2;
        } else {
            res += hchar[i];
        }
    }
}