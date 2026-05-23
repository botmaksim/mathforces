const http = require('http');

function req(method, path, body, headers = {}) {
  return new Promise((resolve, reject) => {
    const opts = {
      hostname: '127.0.0.1',
      port: 3000,
      path: path,
      method: method,
      headers: headers
    };
    if (body) {
      opts.headers['Content-Type'] = 'application/json';
      body = JSON.stringify(body);
      opts.headers['Content-Length'] = Buffer.byteLength(body);
    }
    const r = http.request(opts, (res) => {
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => resolve({code: res.statusCode, body: data}));
    });
    r.on('error', reject);
    if (body) r.write(body);
    r.end();
  });
}

(async () => {
    console.log("Logging in");
    let res = await req("POST", "/api/login/email", {email: "superadmin@example.com", password: "superadmin_pass"});
    console.log(res);
    let token = JSON.parse(res.body).token;
    
    console.log("Fetching users");
    res = await req("GET", "/api/users", null, {"Authorization": token});
    console.log(res.code);
    let users = JSON.parse(res.body);
    console.log(users.map(u => u.username));
    
    let id = users.find(u => u.username === 'student').id;
    console.log("Banning id " + id);
    res = await req("POST", "/api/users/ban", {user_id: id, is_banned: true}, {"Authorization": token});
    console.log(res);
})();
