const fs = require('fs')

module.exports = {
  sayHello: () => {
    fs.writeFileSync('hello.txt', 'Hello!')
  },
}
