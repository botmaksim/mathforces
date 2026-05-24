const https = require('https');

https.get('https://raw.githubusercontent.com/userver-framework/userver/develop/README.md', (res) => {
  let data = '';
  res.on('data', (chunk) => { data += chunk; });
  res.on('end', () => { console.log(data); });
});
